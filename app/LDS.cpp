#include <iostream>
#include <memory>

#include "LocalStorage.hpp"
#include "NBDDriverComm.hpp"
#include "reactor.hpp"

using namespace hrd41;

void HandleRequest(NBDDriverComm& driver, LocalStorage& storage, Reactor& reactor) {
  auto request = driver.ReceiveRequest();

  std::cout << "Got request: type=" << request->m_type
            << " offset=" << request->m_offset
            << " len=" << request->m_buffer.size() << std::endl;

  switch (request->m_type)
  {
  case DriverData::READ:
    std::cout << "READ" << std::endl;
    storage.Read(request);
    driver.SendReplay(request);
    break;

  case DriverData::WRITE:
    std::cout << "WRITE" << std::endl;
    storage.Write(request);
    driver.SendReplay(request);
    break;

  case DriverData::FLUSH:
    request->m_status = DriverData::SUCCESS;
    driver.SendReplay(request);
    break;

  case DriverData::TRIM:
    request->m_status = DriverData::SUCCESS;
    driver.SendReplay(request);
    break;

  case DriverData::DISCONNECT:
    std::cout << "BUSE: disconnect received" << std::endl;
    reactor.Stop();
    break;
  }
}

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
    Reactor reactor;

    std::cout << "BUSE: serving " << size << " bytes on " << device
              << std::endl;

    reactor.Add(driver.GetFD());
    reactor.SetHandler([&]([[maybe_unused]] int fd) {
      std::cout << "Waiting for request..." << std::endl;
      HandleRequest(driver, storage, reactor);
    });
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
