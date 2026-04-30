#include "reactor.hpp"
#include <sys/signalfd.h>

namespace hrd41 {

Reactor::Reactor() : epoll_fd(epoll_create1(0)), signal_fd(-1), running(false) {
  if (epoll_fd == -1) {
    throw ReactorError("Failed to create epoll");
  }

  SetupSignals();
}

Reactor::~Reactor() {
  if (signal_fd != -1) {
    close(signal_fd);
  }
  if (epoll_fd != -1) {
    close(epoll_fd);
  }
}

void Reactor::SetupSignals() {
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);

  if (sigprocmask(SIG_BLOCK, &mask, nullptr) == -1) {
    throw ReactorError("Failed to block signals");
  }

  signal_fd = signalfd(-1, &mask, SFD_CLOEXEC);
  if (signal_fd == -1) {
    throw ReactorError("Failed to create signalfd");
  }

  epoll_event event = {};
  event.events = EPOLLIN;
  event.data.fd = signal_fd;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, signal_fd, &event) == -1) {
    close(signal_fd);
    throw ReactorError("Failed to add signal_fd to epoll");
  }
}

void Reactor::Add(int fd) {
  epoll_event event = {};
  event.events = EPOLLIN;
  event.data.fd = fd;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1) {
    throw ReactorError("Failed to add fd to epoll");
  }
}

void Reactor::Remove(int fd) {
  if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr) == -1) {
    throw ReactorError("Failed to remove fd from epoll");
  }
}

void Reactor::SetHandler(std::function<void(int)> handler) {
  io_handler = handler;
}

void Reactor::Run() {
  if (!io_handler) {
    throw ReactorError("No handler set. Call SetHandler() first");
  }

  running = true;

  while (running) {
    epoll_event events[MAX_EVENTS];

    int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (n == -1) {
      if (errno == EINTR)
        continue;
      throw ReactorError("epoll_wait failed");
    }

    for (int i = 0; i < n; ++i) {
      if (events[i].data.fd == signal_fd) {
        running = false;
      } else {
        io_handler(events[i].data.fd);
      }
    }
  }
}

void Reactor::Stop() {
  running = false;
}

} // namespace hrd41
