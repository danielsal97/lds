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
    std::cerr << "Usage: " << prog << " <mode> <params> <num-minions> [backend] [backend-path]\n\n"
              << "Modes:\n"
              << "  nbd <nbd-device> <size-bytes> [num-minions] [backend] [backend-path]\n"
              << "    Example: " << prog << " nbd /dev/nbd0 134217728 4 memory\n"
              << "    Example: " << prog << " nbd /dev/nbd0 134217728 4 file ./minion_data\n\n"
              << "  tcp <port> <size-bytes> [num-minions] [backend] [backend-path]\n"
              << "    Example: " << prog << " tcp 9999 134217728 4 memory\n"
              << "    Example: " << prog << " tcp 9999 134217728 4 file ./minion_data\n\n"
              << "Minion backends: memory (default), file\n"
              << "num-minions: defaults to hardware_concurrency if not specified\n";
}

size_t ParseSize(const char* value)
{
    return static_cast<size_t>(std::stoull(value));
}

int ParsePort(const char* value)
{
    return std::stoi(value);
}

MinionBackend ParseBackend(const char* value)
{
    std::string backend_str(value);
    if (backend_str == "memory")
        return MinionBackend::MEMORY;
    if (backend_str == "file")
        return MinionBackend::FILE;
    throw std::runtime_error("Unknown minion backend: " + backend_str);
}

template <typename Driver>
void RunServer(Driver& driver, IStorage& storage)
{
    ThreadPool pool;
    Reactor reactor;
    InputMediator mediator(&driver, &storage, &pool, &reactor);
    PNP pnp("plugins/");

    reactor.Add(driver.GetFD());
    reactor.SetHandler([&mediator](int fd) {
        mediator.Notify(fd);
    });

    reactor.Run();
}

int RunNBDMode(const std::string& device, size_t size, size_t num_minions = 0,
               MinionBackend backend = MinionBackend::MEMORY, const std::string& backend_path = "")
{
    RAIDStorage storage(num_minions, size, backend, backend_path);
    NBDDriverComm driver(device, size);

    std::cout << "LDS: NBD mode - serving "
              << size << " bytes on " << device << '\n';

    RunServer(driver, storage);
    return 0;
}

int RunTCPMode(int port, size_t size, size_t num_minions = 0,
               MinionBackend backend = MinionBackend::MEMORY, const std::string& backend_path = "")
{
    RAIDStorage storage(num_minions, size, backend, backend_path);
    TCPDriverComm driver(port);

    std::cout << "LDS: TCP mode - listening on port "
              << port << " (" << size << " bytes storage)\n";

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

        size_t num_minions = (argc >= 5) ? ParseSize(argv[4]) : 0;
        MinionBackend backend = (argc >= 6) ? ParseBackend(argv[5]) : MinionBackend::MEMORY;
        std::string backend_path = (argc >= 7) ? argv[6] : "";
        return RunNBDMode(argv[2], ParseSize(argv[3]), num_minions, backend, backend_path);
    }

    if (mode == "tcp")
    {
        if (argc < 4)
        {
            PrintUsage(argv[0]);
            return 1;
        }

        size_t num_minions = (argc >= 5) ? ParseSize(argv[4]) : 0;
        MinionBackend backend = (argc >= 6) ? ParseBackend(argv[5]) : MinionBackend::MEMORY;
        std::string backend_path = (argc >= 7) ? argv[6] : "";
        return RunTCPMode(ParsePort(argv[2]), ParseSize(argv[3]), num_minions, backend, backend_path);
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