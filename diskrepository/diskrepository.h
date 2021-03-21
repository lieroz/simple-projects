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
    DiskRepository(std::filesystem::path _filename, size_t size) noexcept
        : filename(_filename), pageSize(getpagesize())
    {
        bufferCapacity = ((size / pageSize) + 1) * pageSize;
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
        if (::fallocate(fd, 0, 0, bufferCapacity) == -1)
        {
            return std::make_error_code(static_cast<std::errc>(errno));
        }

        if (buffer = static_cast<char *>(::mmap(
                nullptr, bufferCapacity << 1, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
            buffer == MAP_FAILED)
        {
            return std::make_error_code(static_cast<std::errc>(errno));
        }

        if (::mmap(buffer, bufferCapacity, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0) ==
            MAP_FAILED)
        {
            return std::make_error_code(static_cast<std::errc>(errno));
        }

        if (mmap(buffer + bufferCapacity, bufferCapacity, PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_FIXED, fd, 0) == MAP_FAILED)
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

            fileWriteOffset = ::lseek(backupFile, 0, SEEK_END);
        }

        return std::error_code();
    }

    [[nodiscard]] std::error_code close() noexcept
    {
        if (::munmap(buffer, bufferCapacity << 1) == -1)
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

    template<typename = std::enable_if_t<UseDisk == true>>
    [[nodiscard]] std::error_code flush() noexcept
    {
        auto ec = flushImpl(buffer + readOffset, bufferSize);
        if (!ec)
        {
            bufferSize = 0;
            writeOffset = 0;
            readOffset = 0;
        }
        return ec;
    }

    template<typename = std::enable_if_t<UseDisk == true>>
    [[nodiscard]] std::error_code flush(const Args &...args) noexcept
    {
        size_t offset{0};
        (fillWriteBackBuffer(args, offset), ...);
        auto ec = flushImpl(writeBackBuffer.data(), offset);
        writeBackBuffer.clear();
        return ec;
    }

    template<typename = std::enable_if_t<UseDisk == true>>
    std::pair<std::error_code, size_t> tellDataSize() noexcept
    {
        if (backupFile == -1)
        {
            return {std::make_error_code(std::errc::bad_file_descriptor), 0};
        }

        size_t size{0};
        ::lseek(backupFile, 0, SEEK_SET);

        if (::read(backupFile, reinterpret_cast<char *>(&size), sizeof(size_t)) == -1)
        {
            return {std::make_error_code(static_cast<std::errc>(errno)), 0};
        }

        return {std::error_code(), size};
    }

    template<typename = std::enable_if_t<UseDisk == true>>
    [[nodiscard]] std::error_code refill(size_t size) noexcept
    {
        auto ec = refillImpl(buffer, size);
        if (!ec)
        {
            bufferSize = size;
            writeOffset = size;
            readOffset = 0;
        }
        return ec;
    }

    template<typename = std::enable_if_t<UseDisk == true>>
    [[nodiscard]] std::error_code refill(size_t size, Args &...args) noexcept
    {
        if (size > writeBackBuffer.capacity())
        {
            writeBackBuffer.resize(((size / pageSize) + 1) * pageSize);
        }

        auto ec = refillImpl(writeBackBuffer.data(), size);
        size_t offset{0};
        (dryWriteBackBuffer(args, offset), ...);
        writeBackBuffer.clear();
        return ec;
    }

    void reset() noexcept
    {
        bufferSize = 0;
        writeOffset = 0;
        readOffset = 0;
        fileWriteOffset = 0;
        writeBackBuffer.clear();
    }

    size_t capacity() const noexcept
    {
        return bufferCapacity;
    }

private:
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    [[nodiscard]] bool pushImpl(const T &value, size_t &offset, size_t &size) noexcept
    {
        size_t typeSize = sizeof(T);
        if (size + typeSize > bufferCapacity)
        {
            return false;
        }

        const char *ptr = reinterpret_cast<const char *>(&value);
        std::copy(ptr, ptr + typeSize, buffer + offset);
        offset = (offset + typeSize) % bufferCapacity;
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

        if (size + length > bufferCapacity)
        {
            return false;
        }

        std::copy(value.begin(), value.end(), buffer + offset);
        offset = (offset + length) % bufferCapacity;
        size += length;
        return true;
    }

    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    [[nodiscard]] bool pullImpl(T &value, size_t &offset, size_t &size) noexcept
    {
        size_t typeSize = sizeof(T);
        if (size < typeSize)
        {
            return false;
        }

        const char *ptr = buffer + offset;
        std::copy(ptr, ptr + typeSize, reinterpret_cast<char *>(&value));
        offset = (offset + typeSize) % bufferCapacity;
        size -= typeSize;
        return true;
    }

    [[nodiscard]] bool pullImpl(std::string &value, size_t &offset, size_t &size) noexcept
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
        const char *ptr = buffer + offset;
        std::copy(ptr, ptr + length, value.begin());
        offset = (offset + length) % bufferCapacity;
        size -= length;
        return true;
    }

    template<typename T, typename = std::enable_if_t<std::is_integral_v<T> && UseDisk == true>>
    void fillWriteBackBuffer(const T &value, size_t &offset) noexcept
    {
        size_t typeSize = sizeof(value);
        if (writeBackBuffer.capacity() - offset < typeSize)
        {
            writeBackBuffer.resize(writeBackBuffer.capacity() + pageSize);
        }

        const char *ptr = reinterpret_cast<const char *>(&value);
        std::copy(ptr, ptr + typeSize, writeBackBuffer.begin() + offset);
        offset += typeSize;
    }

    template<typename = std::enable_if_t<UseDisk == true>>
    void fillWriteBackBuffer(const std::string &value, size_t &offset) noexcept
    {
        if (writeBackBuffer.capacity() - offset < value.size())
        {
            writeBackBuffer.resize(
                writeBackBuffer.capacity() + ((value.size() / pageSize + 1) * pageSize));
        }

        fillWriteBackBuffer(value.size(), offset);
        std::copy(value.begin(), value.end(), writeBackBuffer.begin() + offset);
        offset += value.size();
    }

    template<typename T, typename = std::enable_if_t<UseDisk == true>>
    void dryWriteBackBuffer(T &value, size_t &offset)
    {
        auto begin = writeBackBuffer.begin() + offset;
        std::copy(begin, begin + sizeof(T), reinterpret_cast<char *>(&value));
        offset += sizeof(T);
    }

    template<typename = std::enable_if_t<UseDisk == true>>
    void dryWriteBackBuffer(std::string &value, size_t &offset)
    {
        size_t size{0};
        dryWriteBackBuffer(size, offset);

        value.resize(size);
        auto begin = writeBackBuffer.begin() + offset;
        std::copy(begin, begin + size, value.begin());
        offset += value.size();
    }

    template<typename = std::enable_if_t<UseDisk == true>>
    std::error_code flushImpl(const char *ptr, size_t size) noexcept
    {
        if (backupFile == -1)
        {
            return std::make_error_code(std::errc::bad_file_descriptor);
        }

        // TODO: maybe writev here?
        ::lseek(backupFile, fileWriteOffset, SEEK_SET);
        if (::write(backupFile, reinterpret_cast<char *>(&size), sizeof(size_t)) == -1)
        {
            return std::make_error_code(static_cast<std::errc>(errno));
        }

        size_t bytesWritten{0};
        do
        {
            ssize_t bytes = ::write(backupFile, ptr + bytesWritten, size - bytesWritten);
            if (bytes == -1)
            {
                return std::make_error_code(static_cast<std::errc>(errno));
            }
            bytesWritten += bytes;
        } while (bytesWritten < size);

        fileWriteOffset += sizeof(size_t) + bytesWritten;
        return std::error_code();
    }

    template<typename = std::enable_if_t<UseDisk == true>>
    std::error_code refillImpl(char *ptr, size_t size)
    {
        size_t bytesRead{0};
        do
        {
            ssize_t bytes = ::read(backupFile, ptr, size);
            if (bytes == -1)
            {
                return std::make_error_code(static_cast<std::errc>(errno));
            }
            bytesRead += bytes;
        } while (bytesRead < size);

        // TODO: fix me, bytesRead maybe not multiple of file block size
        if (::fallocate(backupFile, FALLOC_FL_COLLAPSE_RANGE, 0, bytesRead) == -1)
        {
            return std::make_error_code(static_cast<std::errc>(errno));
        }

        return std::error_code();
    }

private:
    std::filesystem::path filename;
    size_t pageSize{0};

    size_t bufferCapacity{0};
    size_t bufferSize{0};
    size_t writeOffset{0};
    size_t readOffset{0};
    char *buffer{nullptr};

    int backupFile{-1};
    size_t fileWriteOffset{0};

    std::string writeBackBuffer;
};
