// ============================================================================
// factory.hpp
// What:  Factory pattern for creating objects based on a key
// Why:   Decouples object creation from usage; allows registering different
//        concrete implementations and creating them by key at runtime
// How:   Add(key, constructor_function) registers, Create(key, args) makes objects
// ============================================================================

// function create declaration must be:
// static Base* (*CreateFunc)(Args&)

#ifndef __ILRD_FACTORY_H__
#define __ILRD_FACTORY_H__

#include <functional>    // std::function
#include <memory>        // std::shared_ptr
#include <unordered_map> // std::unordered_map

#include "singelton.hpp"
namespace hrd41
{

template <typename base, typename Key, typename Args> class Factory
{
public:
  Factory(const Factory& o_) = delete;
  Factory& operator=(const Factory& o_) = delete;
  ~Factory() = default;

  using CreateFunc = std::function<std::shared_ptr<base>(Args args)>;

  // add or update
  void Add(const Key& key, CreateFunc createfunc);

  std::shared_ptr<base> Create(const Key& key, Args& args);

private:
  explicit Factory() = default;

  std::unordered_map<Key, CreateFunc> m_createTable;

  friend Singleton<Factory<base, Key, Args>>;

};

template <typename base, typename Key, typename Args>
void Factory<base, Key, Args>::Add(const Key& key, CreateFunc createfunc)
{
    m_createTable[key]  = createfunc;
}

template <typename base, typename Key, typename Args>
std::shared_ptr<base> Factory<base, Key, Args>::Create(const Key& key, Args& args)
{
    return m_createTable.at(key)(args);
}

} // namespace hrd41

#endif // __ILRD_FACTORY_HPP__

