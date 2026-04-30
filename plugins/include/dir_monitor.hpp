// ============================================================================
// dir_monitor.hpp
// What:  Monitors directory for file changes using inotify and broadcasts events
// Why:   Detects when new plugin files appear; inotify runs in background
//        thread; Dispatcher notifies all listeners of file path
// How:   Uses inotify to watch directory; when event occurs, m_disp.NotifyAll()
//        sends file path to observers (e.g., SoLoader)
// ============================================================================

#ifndef __ILRD_DIR_MONITOR_H__
#define __ILRD_DIR_MONITOR_H__

#include <string>
#include <thread>
#include <memory>

#include "logger.hpp"
#include "Dispatcher.hpp"

class InotifyManager;
class InotifyWatch;
class InotifyEventHandler;

namespace hrd41
{

class DirMonitor
{
public:
    explicit DirMonitor(const std::string& m_dirName);
    DirMonitor(const DirMonitor& o_) = delete;
    DirMonitor& operator=(const DirMonitor& o_) = delete;
    ~DirMonitor();

    Dispatcher<const std::string&>* GetDispatcher();
    const std::string& GetDirName() const { return m_dirName; }
    Logger* GetLogger() const { return m_logger; }

private:
    std::string m_dirName;
    std::thread m_listener;
    Dispatcher<const std::string&> m_disp;
    Logger* m_logger;
    std::unique_ptr<InotifyManager> m_manager;
    InotifyWatch* m_watch;
    std::unique_ptr<InotifyEventHandler> m_handler;
};

} // namespace hrd41

#endif // __ILRD_DIR_MONITOR_H__