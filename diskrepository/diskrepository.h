#pragma once

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <cstring>
#include <system_error>
#include <string_view>
#include <filesystem>

template<bool UseDisk = true, typename... Args>
class DiskRepository final
{
public:
    DiskRepository(std::filesystem::path _filename, size_t size) noexcept : filename(_filename)
    {
        size_t pageSize = getpagesize();
        capacity = ((size / pageSize) + 1) * pageSize;
        writeBackBuffer.resize(capacity);
        writeBackBuffer.clear();
    }

    // close() is not called upon error cause there are 2 valid cases of usage
    //  1. use only till program exit, then all files and mappings will be released
    //  2. call close() explicitly if program needs to smth further
    [[nodiscard]] std::error_code open() noexcept
    {
        auto *file = ::tmpfile();
        if (file == nullptr)
        {
            return std::make_error_code(static_cast<std::errc>(errno));
        }

        int fd = ::fileno(file);
        if (::fallocate(fd, 0, 0, capacity) == -1)
        {
            return std::make_error_code(static_cast<std::errc>(errno));
        }

        if (buffer = static_cast<uint8_t *>(
                ::mmap(nullptr, capacity << 1, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
            buffer == MAP_FAILED)
        {
            return std::make_error_code(static_cast<std::errc>(errno));
        }

        if (::mmap(buffer, capacity, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0) ==
            MAP_FAILED)
        {
            return std::make_error_code(static_cast<std::errc>(errno));
        }

        if (mmap(buffer + capacity, capacity, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd,
                0) == MAP_FAILED)
        {
            return std::make_error_code(static_cast<std::errc>(errno));
        }

        if constexpr (UseDisk == true)
        {
            if (backupFile = ::open(filename.c_str(), O_RDWR | O_CREAT | O_SYNC, S_IRUSR | S_IWUSR);
                backupFile == -1)
            {
                return std::make_error_code(static_cast<std::errc>(errno));
            }

            if (int err = ::posix_fadvise(backupFile, 0, 0, POSIX_FADV_SEQUENTIAL); err != 0)
            {
                return std::make_error_code(static_cast<std::errc>(errno));
            }
        }

        return std::error_code();
    }

    [[nodiscard]] std::error_code close() noexcept
    {
        if (::munmap(buffer, capacity << 1) == -1)
        {
            return std::make_error_code(static_cast<std::errc>(errno));
        }

        if constexpr (UseDisk == true)
        {
            if (backupFile != -1 && ::close(backupFile) == -1)
            {
                return std::make_error_code(static_cast<std::errc>(errno));
            }
        }

        return std::error_code();
    }

    [[nodiscard]] bool push(const Args &...args) noexcept
    {
        if (buffer == nullptr)
        {
            return false;
        }

        size_t offset = writeOffset;
        size_t size = bufferSize;
        bool ok{false};

        if (((ok = pushImpl(args, offset, size)), ...); ok)
        {
            writeOffset = offset;
            bufferSize = size;
        }
        return ok;
    }

    [[nodiscard]] bool pull(Args &...args) noexcept
    {
        if (buffer == nullptr)
        {
            return false;
        }

        size_t offset = readOffset;
        size_t size = bufferSize;
        bool ok{false};

        if (((ok = pullImpl(args, offset, size)), ...); ok)
        {
            readOffset = offset;
            bufferSize = size;
        }
        return ok;
    }

    [[nodiscard]] std::error_code flush() noexcept
    {
        auto ec = flushImpl(buffer + readOffset, bufferSize);
        if (!ec)
        {
            commit();
        }
        return ec;
    }

    [[nodiscard]] std::error_code flush(const Args &...args)
    {
        size_t offset{0};
        (fillWriteBackBuffer(args, offset), ...);
        auto ec = flushImpl(reinterpret_cast<uint8_t *>(writeBackBuffer.data()), offset);
        writeBackBuffer.clear();
        return ec;
    }

    void commit()
    {
        bufferSize = 0;
        writeOffset = 0;
        readOffset = 0;
    }

private:
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    [[nodiscard]] bool pushImpl(const T &value, size_t &offset, size_t &size) noexcept
    {
        size_t typeSize = sizeof(T);
        if (size + typeSize > capacity)
        {
            return false;
        }

        const uint8_t *ptr = reinterpret_cast<const uint8_t *>(&value);
        std::copy(ptr, ptr + typeSize, buffer + offset);
        offset = (offset + typeSize) % capacity;
        size += typeSize;
        return true;
    }

    [[nodiscard]] bool pushImpl(const std::string &value, size_t &offset, size_t &size) noexcept
    {
        size_t length = value.length();
        if (!pushImpl(length, offset, size))
        {
            return false;
        }

        if (size + length > capacity)
        {
            return false;
        }

        std::copy(value.begin(), value.end(), buffer + offset);
        offset = (offset + length) % capacity;
        size += length;
        return true;
    }

    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    [[nodiscard]] bool pullImpl(T &value, size_t &offset, size_t &size)
    {
        size_t typeSize = sizeof(T);
        if (size < typeSize)
        {
            return false;
        }

        const uint8_t *ptr = buffer + offset;
        std::copy(ptr, ptr + typeSize, reinterpret_cast<uint8_t *>(&value));
        offset = (offset + typeSize) % capacity;
        size -= typeSize;
        return true;
    }

    [[nodiscard]] bool pullImpl(std::string &value, size_t &offset, size_t &size)
    {
        size_t length{0};
        if (!pullImpl(length, offset, size))
        {
            return false;
        }

        if (size < length)
        {
            return false;
        }

        value.resize(length);
        const uint8_t *ptr = buffer + offset;
        std::copy(ptr, ptr + length, value.begin());
        offset = (offset + length) % capacity;
        size -= length;
        return true;
    }

    template<typename T, typename = std::enable_if_t<std::is_integral_v<T> && UseDisk == true>>
    void fillWriteBackBuffer(const T &value, size_t &offset)
    {
        size_t typeSize = sizeof(value);
        if (writeBackBuffer.capacity() - offset < typeSize)
        {
            writeBackBuffer.resize(writeBackBuffer.capacity() << 1);
        }

        const char *ptr = reinterpret_cast<const char *>(&value);
        std::copy(ptr, ptr + typeSize, writeBackBuffer.begin() + offset);
        offset += typeSize;
    }

    template<typename = std::enable_if_t<UseDisk == true>>
    void fillWriteBackBuffer(const std::string &value, size_t &offset)
    {
        if (writeBackBuffer.capacity() - offset < value.size())
        {
            writeBackBuffer.resize(writeBackBuffer.capacity() << 1);
        }

        fillWriteBackBuffer(value.size(), offset);
        std::copy(value.begin(), value.end(), writeBackBuffer.begin() + offset);
        offset += value.size();
    }

    template<typename = std::enable_if_t<UseDisk == true>>
    std::error_code flushImpl(const uint8_t *ptr, size_t size)
    {
        if (backupFile == -1)
        {
            return std::make_error_code(std::errc::bad_file_descriptor);
        }

        ssize_t bytesWritten{0};
        ::lseek(backupFile, 0, SEEK_END);

        do
        {
            ssize_t bytes = ::write(backupFile, ptr + bytesWritten, size - bytesWritten);
            if (bytes == -1)
            {
                ::lseek(backupFile, -bytesWritten, SEEK_CUR);
                return std::make_error_code(static_cast<std::errc>(errno));
            }
            bytesWritten += bytes;
        } while (static_cast<size_t>(bytesWritten) < size);

        return std::error_code();
    }

    template<typename = std::enable_if_t<UseDisk == true>>
    std::error_code refillImpl()
    {
        if (backupFile == -1)
        {
            return std::make_error_code(std::errc::bad_file_descriptor);
        }

        return std::error_code();
    }

private:
    std::filesystem::path filename;
    size_t capacity{0};
    size_t bufferSize{0};
    size_t writeOffset{0};
    size_t readOffset{0};
    uint8_t *buffer{nullptr};
    int backupFile{-1};
    std::string writeBackBuffer;
};
