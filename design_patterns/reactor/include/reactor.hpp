// ============================================================================
// reactor.hpp
// What:  Reactor pattern using epoll for event-driven I/O multiplexing
// Why:   Efficiently handles multiple file descriptors (driver requests,
//        signals) without blocking; single thread processes all events
// How:   Add(fd) registers file descriptor; SetHandler() sets callback for
//        all events; Run() blocks on epoll_wait, calls handler when events occur
// ============================================================================

#ifndef __ILRD_REACTOR_HPP__
#define __ILRD_REACTOR_HPP__

#include <sys/epoll.h>
#include <unistd.h>
#include <functional>
#include <stdexcept>
#include <cerrno>
#include <atomic>

namespace hrd41 {

class Reactor {
public:
  explicit Reactor(); 
  ~Reactor();

  Reactor(const Reactor&) = delete;
  Reactor& operator=(const Reactor&) = delete;

  void Add(int fd);
  void Remove(int fd);
  void SetHandler(std::function<void(int)> handler);
  void Run();

private:
  int m_epoll_fd;
  int m_signal_fd;
  std::function<void(int)> m_io_handler;
  static constexpr int MAX_EVENTS = 10;

  void SetupSignals();

  class ReactorError : public std::runtime_error {
  public:
    explicit ReactorError(const std::string& msg) : std::runtime_error(msg) {}
  };
};

} // namespace hrd41

#endif // __ILRD_REACTOR_HPP__
