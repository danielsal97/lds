#pragma once

#include <thread>
#include <memory>
#include <sys/types.h>

namespace hrd41 {

class WatchdogMonitor {
public:
  // Start monitoring the watchdog (parent process)
  // If watchdog dies, restart it with the given arguments
  static void StartMonitoring(int argc, char* argv[]);

  // Stop monitoring
  static void StopMonitoring();

private:
  static std::unique_ptr<std::thread> monitor_thread;
  static volatile bool is_monitoring;
  static pid_t watchdog_pid;

  // Monitor thread function
  static void MonitorWatchdog(int argc, char* argv[]);

  // Restart watchdog with same arguments
  static void RestartWatchdog(int argc, char* argv[]);
};

} // namespace hrd41
