#include "dir_monitor.hpp"
#include "InotifyEvent.h"
#include "InotifyEventHandler.h"
#include "InotifyManager.h"
#include "InotifyWatch.h"
#include "singelton.hpp"

namespace hrd41
{

class DirMonitorHandler : public InotifyEventHandler
{
private:
  DirMonitor* m_monitor;

public:
  DirMonitorHandler(DirMonitor* monitor) : m_monitor(monitor) {}

  bool handle(InotifyEvent& e) override
  {
    std::string filename = e.getName();
    if (filename.find(".so") != std::string::npos)
    {
      std::string fullPath = m_monitor->GetDirName() + "/" + filename;
      m_monitor->GetLogger()->Write("Plugin detected: " + fullPath,
                                    Logger::INFO);
      m_monitor->GetDispatcher()->NotifyAll(fullPath);
    }
    return false;
  }
};

DirMonitor::DirMonitor(const std::string& dirName)
    : m_dirName(dirName), m_disp(), m_logger(Singleton<Logger>::GetInstance()),
      m_manager(std::make_unique<InotifyManager>()), m_watch(nullptr)
{
  m_watch = m_manager->addWatch(dirName, IN_CREATE | IN_CLOSE_WRITE);
  if (!m_watch)
  {
    m_logger->Write("Failed to add watch for: " + dirName, Logger::ERROR);
    return;
  }

  m_handler = std::make_unique<DirMonitorHandler>(this);
  m_watch->addEventHandler(*m_handler);

  m_logger->Write("Watching directory: " + dirName, Logger::INFO);
  m_listener = m_manager->startWatching();
}

DirMonitor::~DirMonitor()
{
  if (m_listener.joinable())
  {
    m_listener.join();
  }
}

Dispatcher<const std::string&>* DirMonitor::GetDispatcher() 
{
     return &m_disp; 
}


} // namespace hrd41
