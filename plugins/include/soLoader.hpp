// ============================================================================
// soLoader.hpp
// What:  Loads .so plugin files and registers them with system
// Why:   Integrates dynamically-loaded plugins using Loader; Dispatcher
//        notifies SoLoader when new files appear
// How:   Listens to Dispatcher; when file path arrives, Load(libPath) opens
//        .so via Loader and keeps handle in m_libs vector
// ============================================================================

#ifndef __ILRD_SO_LOADER_H__
#define __ILRD_SO_LOADER_H__

#include <vector>           // std::vector
#include <string>           // std::string

#include "logger.hpp"       // Logger
#include "Dispatcher.hpp"
#include "CallBack.hpp"


namespace hrd41
{

class SoLoader
{
public:
    explicit SoLoader(Dispatcher<const std::string&>* disp);
    SoLoader(const SoLoader& o_) = delete;
    SoLoader& operator=(const SoLoader& o_) = delete;
    ~SoLoader();

    void Load(const std::string& libPath);

private:
    using PlugInCB = CallBack<const std::string&, SoLoader>;

    std::vector<void*> m_libs;
    PlugInCB* m_pluginCB;
    Logger* m_logger;

    void OnLoad(const std::string& libPath);
};

} // namespace hrd41

#endif //__ILRD_SO_LOADER_H__
