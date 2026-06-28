#include <iostream>
#include <memory>
#include <string>
#include <stdexcept>

#include "RAIDManager.hpp"
#include "NBDDriverComm.hpp"
#include "TCPDriverComm.hpp"
#include "InputMediator.hpp"
#include "reactor.hpp"
#include "thread_pool.hpp"
#include "pnp.hpp"

using namespace hrd41;

namespace
{

void PrintUsage(const char* prog)
{
    std::cerr << "Usage: " << prog << " <mode> [options]\n\n"
              << "Modes:\n"
              << "  nbd <nbd-device> <size-bytes> [num-minions] [minion-size-mb]\n"
              << "    Example: " << prog << " nbd /dev/nbd0 134217728\n"
              << "    Example: " << prog << " nbd /dev/nbd0 134217728 4 10\n\n"
              << "  tcp <port> <size-bytes> [num-minions] [minion-size-mb]\n"
              << "    Example: " << prog << " tcp 9999 134217728\n"
              << "    Example: " << prog << " tcp 9999 134217728 4 10\n"
              << "  Default: num-minions = CPU cores, minion-size = 5 MB\n";
}

size_t ParseSize(const char* value)
{
    return static_cast<size_t>(std::stoull(value));
}

int ParsePort(const char* value)
{
    return std::stoi(value);
}

template <typename Driver>
void RunServer(Driver& driver, IStorage& storage)
{
    ThreadPool pool;
    InputMediator mediator(&driver, &storage, &pool);
    Reactor reactor;
    PNP pnp("plugins/");

    reactor.Add(driver.GetFD());
    reactor.SetHandler([&mediator](int fd) {
        mediator.Notify(fd);
    });

    reactor.Run();
}

int RunNBDMode(const std::string& device, size_t size, size_t num_minions = 0,
               size_t minion_size = 5 * 1024 * 1024)
{
    RAIDStorage storage(num_minions, minion_size);
    NBDDriverComm driver(device, size);

    std::cout << "LDS: NBD mode - serving "
              << size << " bytes on " << device << '\n';
    std::cout << "LDS: RAID 01 storage - " << storage.GetNumMinions()
              << " minions (" << storage.GetMinionsPerSet() << " per set)\n";

    RunServer(driver, storage);
    return 0;
}

int RunTCPMode(int port, size_t size, size_t num_minions = 0,
               size_t minion_size = 5 * 1024 * 1024)
{
    RAIDStorage storage(num_minions, minion_size);
    TCPDriverComm driver(port);

    std::cout << "LDS: TCP mode - listening on port "
              << port << " (" << size << " bytes storage)\n";
    std::cout << "LDS: RAID 01 storage - " << storage.GetNumMinions()
              << " minions (" << storage.GetMinionsPerSet() << " per set)\n";

    std::cout << "LDS: Waiting for client connection...\n";

    RunServer(driver, storage);
    return 0;
}

int DispatchMode(int argc, char* argv[])
{
    if (argc < 2)
    {
        PrintUsage(argv[0]);
        return 1;
    }

    const std::string mode = argv[1];

    if (mode == "nbd")
    {
        if (argc < 4)
        {
            PrintUsage(argv[0]);
            return 1;
        }

        size_t num_minions = 0;               // Default to hw_concurrency
        size_t minion_size = 5 * 1024 * 1024; // Default 5MB

        if (argc >= 5)
        {
            num_minions = ParseSize(argv[4]);
        }
        if (argc >= 6)
        {
            minion_size = ParseSize(argv[5]) * 1024 * 1024; // Convert MB to bytes
        }

        return RunNBDMode(argv[2], ParseSize(argv[3]), num_minions, minion_size);
    }

    if (mode == "tcp")
    {
        if (argc < 4)
        {
            PrintUsage(argv[0]);
            return 1;
        }

        size_t num_minions = 0;               // Default to hw_concurrency
        size_t minion_size = 5 * 1024 * 1024; // Default 5MB

        if (argc >= 5)
        {
            num_minions = ParseSize(argv[4]);
        }
        if (argc >= 6)
        {
            minion_size = ParseSize(argv[5]) * 1024 * 1024; // Convert MB to bytes
        }

        return RunTCPMode(ParsePort(argv[2]), ParseSize(argv[3]), num_minions,
                         minion_size);
    }

    PrintUsage(argv[0]);
    return 1;
}

} // namespace

int main(int argc, char* argv[])
{
    try
    {
        const int status = DispatchMode(argc, argv);

        if (status == 0)
        {
            std::cout << "LDS: shutdown complete\n";
        }

        return status;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}