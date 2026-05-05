#include "design_patterns/reactor/include/reactor.hpp"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <chrono>
#include <signal.h>

using namespace hrd41;

int main() {
  std::cout << "=== Reactor Test ===" << std::endl;

  try {
    Reactor reactor;

    // Create a pipe for testing
    int pipefd[2];
    if (pipe(pipefd) == -1) {
      std::cerr << "Failed to create pipe" << std::endl;
      return 1;
    }

    int read_fd = pipefd[0];
    int write_fd = pipefd[1];

    // Set handler
    int event_count = 0;
    reactor.SetHandler([&](int fd) {
      if (fd == read_fd) {
        char buffer[256];
        ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
          buffer[n] = '\0';
          std::cout << "Received: " << buffer << std::endl;
          event_count++;
        }
      }
    });

    // Add pipe to reactor
    reactor.Add(read_fd);

    // Write to pipe from another thread
    std::thread writer([write_fd]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      write(write_fd, "Message 1\n", 10);

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      write(write_fd, "Message 2\n", 10);

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      write(write_fd, "Message 3\n", 10);

      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      kill(getpid(), SIGINT);  // Send signal to exit reactor
    });

    std::cout << "Starting reactor..." << std::endl;
    reactor.Run();
    std::cout << "Reactor stopped" << std::endl;

    writer.join();
    close(read_fd);
    close(write_fd);

    std::cout << "✓ Test passed!" << std::endl;
    return 0;

  } catch (const std::exception& e) {
    std::cerr << "✗ Test failed: " << e.what() << std::endl;
    return 1;
  }
}
