#include <iostream>
#include <memory>

#include "LocalStorage.hpp"
#include "NBDDriverComm.hpp"
#include "InputMediator.hpp"
#include "reactor.hpp"

using namespace hrd41;

int main(int argc, char* argv[])
{
  if (argc < 3)
  {
    std::cerr << "Usage: " << argv[0] << " <nbd-device> <size-bytes>"
              << std::endl;
    std::cerr << "Example: " << argv[0] << " /dev/nbd0 134217728" << std::endl;
    return 1;
  }

  const std::string device = argv[1];
  size_t size = std::stoull(argv[2]);

  try
  {
    LocalStorage storage(size);
    NBDDriverComm driver(device, size);
    InputMediator mediator(&driver, &storage);
    Reactor reactor;

    std::cout << "BUSE: serving " << size << " bytes on " << device
              << std::endl;

    reactor.Add(driver.GetFD());
    reactor.SetHandler([&](int fd) { mediator.Notify(fd); });
    reactor.Run();
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  std::cout << "BUSE: shutdown complete" << std::endl;
  return 0;
}
