// ============================================================================
// pnp.hpp (Plug and Play)
// What:  Monitors plugin directory and dynamically loads new .so files
// Why:   Allows extending system with plugins without recompiling; DirMonitor
//        detects new files, SoLoader loads them at runtime
// How:   DirMonitor watches directory (default "plugins/"), when file appears,
//        SoLoader loads it and integrates into running system
// ============================================================================

#ifndef __ILRD_PNP_H__
#define __ILRD_PNP_H__

#include <string> // std::string

#include "dir_monitor.hpp"

#include "logger.hpp"   // Logger
#include "soLoader.hpp" // SoLoader

namespace hrd41
{

class PNP
{
public:
  explicit PNP(const std::string& dirPath = "plugins/");
  PNP(const PNP& o_) = delete;
  PNP& operator=(const PNP& o_) = delete;
  ~PNP();

private:
  DirMonitor m_dirMonitor;
  SoLoader m_soLoader;
  Logger* m_logger;
};

} // namespace hrd41

#endif //__ILRD_PNP_H__