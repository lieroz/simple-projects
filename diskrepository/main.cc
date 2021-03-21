#include <iostream>
#include <thread>

#include "diskrepository.h"

int main()
{
    DiskRepository<true, std::string> repository{"test", 4000};
    [[maybe_unused]] auto _ = repository.open();

    std::cout << repository.push("Hello, World!") << std::endl;
    std::cout << repository.push(std::string(4000, 'A')) << std::endl;

    std::string data;
    std::cout << repository.pull(data) << std::endl;
    std::cout << data << std::endl;

    std::cout << repository.push(std::string(70, 'B')) << std::endl;

    std::cout << repository.pull(data) << std::endl;
    std::cout << data << " " << data.size() << std::endl;

    std::cout << repository.pull(data) << std::endl;
    std::cout << data << " " << data.size() << std::endl;

    std::cout << repository.push(std::string(1000, 'C')) << std::endl;
    std::cout << repository.pull(data) << std::endl;
    std::cout << data << " " << data.size() << std::endl;

    [[maybe_unused]] auto _f = repository.flush(std::string(5000, 'D'));

    return 0;
}
