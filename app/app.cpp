#include "reactor.hpp"

int main()
{
  // Phase 1: Infrastructure
  hrd41::Reactor reactor;

  // Phase 2: Set handler for driver events
  reactor.SetHandler([](int fd) {
    // Handle driver fd events
    // mediator.HandleEvent(fd);
  });

  // Phase 3: Register file descriptors
  // int driver_fd = open("/dev/nbd0", O_RDONLY);
  // reactor.Add(driver_fd);

  // Start event loop (blocks forever until signal)
  reactor.Run();

  return 0;
}