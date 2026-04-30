#include <dlfcn.h>
#include <stdexcept>

#include "soLoader.hpp"
#include "CallBack.hpp"
#include "singelton.hpp"

namespace hrd41
{

SoLoader::SoLoader(Dispatcher<const std::string&>* disp)
    : m_libs(), m_logger(Singleton<Logger>::GetInstance())
{
    if (!disp) {
        m_logger->Write("Dispatcher is null", Logger::ERROR);
        m_pluginCB = nullptr;
        return;
    }

    m_pluginCB = new CallBack<const std::string&, SoLoader>(
        disp, *this, &SoLoader::OnLoad
    );
    m_logger->Write("SoLoader initialized", Logger::INFO);
}

SoLoader::~SoLoader()
{
    for (void* lib : m_libs) {
        dlclose(lib);
    }
    delete m_pluginCB;
}

void SoLoader::Load(const std::string& libPath)
{
    OnLoad(libPath);
}

void SoLoader::OnLoad(const std::string& libPath)
{
    m_logger->Write("Loading plugin: " + libPath, Logger::INFO);

    void* handle = dlopen(libPath.c_str(), RTLD_LAZY);
    if (!handle) {
        std::string error = "dlopen failed: ";
        error += dlerror();
        m_logger->Write(error, Logger::ERROR);
        throw std::runtime_error(error);
    }

    m_libs.push_back(handle);
    m_logger->Write("Plugin loaded successfully: " + libPath, Logger::INFO);
}

} // namespace hrd41
