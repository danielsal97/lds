#include "reactor.hpp"
#include <sys/signalfd.h>
#include <signal.h>
#include <cstring>
#include <cerrno>

namespace hrd41 {

Reactor::Reactor()
  : m_epoll_fd(epoll_create1(0)), m_signal_fd(-1) {
  if (m_epoll_fd == -1) {
    throw ReactorError("Failed to create epoll");
  }
  SetupSignals();
}

Reactor::~Reactor() {
  if (m_signal_fd != -1) {
    close(m_signal_fd);
  }
  if (m_epoll_fd != -1) {
    close(m_epoll_fd);
  }
}

void Reactor::SetupSignals() {
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);

  if (-1 == sigprocmask(SIG_BLOCK, &mask, nullptr) ) {
    throw ReactorError("Failed to block signals");
  }

  m_signal_fd = signalfd(-1, &mask, SFD_CLOEXEC);
  if (-1 == m_signal_fd) {
    throw ReactorError("Failed to create signalfd");
  }

  epoll_event event = {};
  event.events = EPOLLIN;
  event.data.fd = m_signal_fd;

  if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_signal_fd, &event) == -1) {
    close(m_signal_fd);
    throw ReactorError("Failed to add signal_fd to epoll");
  }
}

void Reactor::Add(int fd) {
  epoll_event event = {};
  event.events = EPOLLIN;
  event.data.fd = fd;

  if (-1 == epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event) ) {
    throw ReactorError("Failed to add fd to epoll");
  }
}

void Reactor::Remove(int fd) {
  if (epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr) == -1) {
    throw ReactorError("Failed to remove fd from epoll");
  }
}

void Reactor::SetHandler(std::function<void(int)> handler) {
  m_io_handler = handler;
}

void Reactor::Run() {
  if (!m_io_handler) {
    throw ReactorError("No handler set. Call SetHandler() first");
  }

  epoll_event events[MAX_EVENTS];

  while (true) {
    int n = epoll_wait(m_epoll_fd, events, MAX_EVENTS, -1);
    if (n == -1) {
      if (errno == EINTR)
        continue;
      throw ReactorError("epoll_wait failed");
    }

    for (int i = 0; i < n; ++i) {
      int fd = events[i].data.fd;

      if (fd == m_signal_fd) {
        return;
      }

      m_io_handler(fd);
    }
  }
}

} // namespace hrd41
