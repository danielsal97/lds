// ============================================================================
// loader.hpp
// What:  RAII wrapper for dlopen() - loads shared object (.so) files
// Why:   Ensures dlclose() is called on destruction; prevents resource leaks
//        when loading plugins dynamically
// How:   Loader(path) opens .so file via dlopen(); destructor calls dlclose()
// ============================================================================

#ifndef __ILRD_LOADER_HPP__
#define __ILRD_LOADER_HPP__

#include <dlfcn.h>
#include <string>
#include <stdexcept>



namespace hrd41
{

class Loader
{
public:
  explicit Loader(const std::string& path_)
    : m_handle(dlopen(path_.c_str(), RTLD_LAZY | RTLD_DEEPBIND))
  {
    if (!m_handle) {
      throw std::runtime_error(std::string("Failed to load: ") + dlerror());
    }
  }

  ~Loader() {
    if (m_handle) {
      dlclose(m_handle);
    }
  }

  Loader(const Loader& other) = delete;
  Loader& operator=(const Loader& other) = delete;

private:
  void* m_handle;
};
} // namespace hrd41
#endif