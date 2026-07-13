#include "WatchdogMonitor.hpp"

#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

namespace hrd41 {

std::unique_ptr<std::thread> WatchdogMonitor::monitor_thread = nullptr;
volatile bool WatchdogMonitor::is_monitoring = false;
pid_t WatchdogMonitor::watchdog_pid = 0;

void WatchdogMonitor::StartMonitoring(int argc, char* argv[])
{
  if (is_monitoring) return;

  // Capture the watchdog PID before parent can change
  watchdog_pid = getppid();

  is_monitoring = true;
  monitor_thread = std::make_unique<std::thread>(
      &WatchdogMonitor::MonitorWatchdog, argc, argv);
  monitor_thread->detach();

  std::cerr << "[WatchdogMonitor] Started monitoring watchdog (PID "
            << watchdog_pid << ")\n";
}

void WatchdogMonitor::StopMonitoring()
{
  is_monitoring = false;
  if (monitor_thread && monitor_thread->joinable()) {
    monitor_thread->join();
  }
  std::cerr << "[WatchdogMonitor] Stopped monitoring\n";
}

void WatchdogMonitor::MonitorWatchdog(int argc, char* argv[])
{
  int check_interval = 3;  // Check every 3 seconds if watchdog is alive

  std::cerr << "[WatchdogMonitor] Monitoring watchdog (PID "
            << watchdog_pid << ")\n";

  while (is_monitoring) {
    sleep(check_interval);

    // Test if watchdog process is alive by sending signal 0
    // This doesn't actually send a signal, just checks if process exists
    int rc = kill(watchdog_pid, 0);

    if (rc == -1) {
      // Watchdog is dead!
      std::cerr << "[WatchdogMonitor] ⚠️  Watchdog died (PID "
                << watchdog_pid << "), restarting...\n";
      RestartWatchdog(argc, argv);
      break;  // Exit monitoring after restart attempt
    }
  }

  std::cerr << "[WatchdogMonitor] Monitor thread exiting\n";
}

void WatchdogMonitor::RestartWatchdog(int argc, char* argv[])
{
  pid_t respawn_pid = fork();

  if (respawn_pid == -1) {
    std::cerr << "[WatchdogMonitor] Fork failed\n";
    return;
  }

  if (respawn_pid == 0) {
    // Child process: restart the watchdog
    std::cerr << "[WatchdogMonitor] ✅ Restarting watchdog: " << argv[0] << "\n";

    // argv[0] should be the watchdog executable (e.g., "./lds_watchdog")
    execvp(argv[0], argv);

    // Only reached on exec failure
    std::cerr << "[WatchdogMonitor] ❌ execvp failed\n";
    exit(1);
  } else {
    // Parent: log the restart
    std::cerr << "[WatchdogMonitor] ✅ Watchdog respawned (new PID "
              << respawn_pid << ")\n";
  }
}

} // namespace hrd41
