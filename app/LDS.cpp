#include <iostream>
#include <memory>
#include <string>

#include "LocalStorage.hpp"
#include "NBDDriverComm.hpp"
#include "TCPDriverComm.hpp"
#include "IDriverComm.hpp"
#include "InputMediator.hpp"
#include "reactor.hpp"

using namespace hrd41;

void print_usage(const char* prog)
{
  std::cerr << "Usage: " << prog << " <mode> [options]" << std::endl;
  std::cerr << std::endl;
  std::cerr << "Modes:" << std::endl;
  std::cerr << "  nbd <nbd-device> <size-bytes>" << std::endl;
  std::cerr << "    Example: " << prog << " nbd /dev/nbd0 134217728" << std::endl;
  std::cerr << std::endl;
  std::cerr << "  tcp <port> <size-bytes>" << std::endl;
  std::cerr << "    Example: " << prog << " tcp 9999 134217728" << std::endl;
}

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    print_usage(argv[0]);
    return 1;
  }

  const std::string mode = argv[1];

  try
  {
    if (mode == "nbd")
    {
      if (argc < 4)
      {
        print_usage(argv[0]);
        return 1;
      }

      const std::string device = argv[2];
      size_t size = std::stoull(argv[3]);

      LocalStorage storage(size);
      NBDDriverComm driver(device, size);
      InputMediator mediator(&driver, &storage);
      Reactor reactor;

      std::cout << "LDS: NBD mode - serving " << size << " bytes on " << device << std::endl;

      reactor.Add(driver.GetFD());
      reactor.SetHandler([&](int fd) { mediator.Notify(fd); });
      reactor.Run();
    }
    else if (mode == "tcp")
    {
      if (argc < 4)
      {
        print_usage(argv[0]);
        return 1;
      }

      int port = std::stoi(argv[2]);
      size_t size = std::stoull(argv[3]);

      LocalStorage storage(size);
      TCPDriverComm driver(port);
      InputMediator mediator(&driver, &storage);
      Reactor reactor;

      std::cout << "LDS: TCP mode - listening on port " << port << " (" << size << " bytes storage)" << std::endl;
      std::cout << "LDS: Waiting for client connection..." << std::endl;

      reactor.Add(driver.GetFD());
      reactor.SetHandler([&](int fd) { mediator.Notify(fd); });
      reactor.Run();
    }
    else
    {
      print_usage(argv[0]);
      return 1;
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  std::cout << "LDS: shutdown complete" << std::endl;
  return 0;
}
