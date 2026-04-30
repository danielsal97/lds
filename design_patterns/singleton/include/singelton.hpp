// ============================================================================
// singelton.hpp
// What:  Singleton pattern with thread-safe double-checked locking
// Why:   Ensures only one global instance exists; some components (Logger,
//        Factory) need centralized state accessible from anywhere
// How:   GetInstance() returns static T* with lazy initialization; safe for
//        multithreading via atomic + mutex
// ============================================================================

#ifndef __ILRD_SINGLETON_HPP__
#define __ILRD_SINGLETON_HPP__

#include <atomic> // std::atomic
#include <memory> // std::unique_ptr
#include <mutex>  // std:mutex

namespace hrd41
{

template <typename T> class Singleton
{
public:
  Singleton() = delete;
  Singleton(const Singleton&) = delete;
  Singleton& operator=(const Singleton&) = delete;
  ~Singleton() = delete;

  static T* GetInstance();

private:
  static std::atomic<T*> s_instance;
  static std::unique_ptr<T> s_owner;
  static std::mutex s_mutex;
};

template <typename T> std::atomic<T*> Singleton<T>::s_instance(nullptr);

template <typename T> std::unique_ptr<T> Singleton<T>::s_owner;

template <typename T> std::mutex Singleton<T>::s_mutex;

template <typename T> T* Singleton<T>::GetInstance()
{
  T* tmp = s_instance.load(std::memory_order_acquire);

  if (nullptr == tmp)
  {
    std::lock_guard<std::mutex> lock(s_mutex);
    tmp = s_instance.load(std::memory_order_relaxed);

    if (nullptr == tmp)
    {
      s_owner.reset(new T());
      tmp = s_owner.get();
      s_instance.store(tmp, std::memory_order_release);
    }
  }

  return tmp;
}

} // namespace hrd41

#endif
