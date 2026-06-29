// ============================================================================
// InputMediator.hpp
// What:  Concrete mediator implementation using lambda handlers
// Why:   Coordinates between Reactor events and RAID Manager operations;
//        maps command types to handler lambdas for clean dispatch
// How:   Stores map of CommandType → lambda functions; Notify() receives
//        file descriptor, extracts request from driver, looks up handler,
//        executes (calls RAID Manager, sends response)
// ============================================================================
#ifndef __ILRD_INPUT_MEDIATOR_HPP__
#define __ILRD_INPUT_MEDIATOR_HPP__

#include <map>
#include <functional>
#include <memory>

#include "IMediator.hpp"

#include "IDriverComm.hpp"
#include "IStorage.hpp"
#include "thread_pool.hpp"
#include "reactor.hpp"

namespace hrd41
{


struct DriverData;

class InputMediator : public IMediator
{
public:
  explicit InputMediator(IDriverComm* driver, IStorage* storage, ThreadPool* pool, Reactor* reactor);

  InputMediator(const InputMediator& o_) = delete;
  InputMediator& operator=(const InputMediator& o_) = delete;
  ~InputMediator() = default;

  void Notify(int fd) override;

private:
  IDriverComm* m_driver;
  IStorage* m_storage;
  ThreadPool* m_pool;
  Reactor* m_reactor;

  std::map<int, std::function<void(std::shared_ptr<DriverData>)>> m_handlers;

  void SetupHandlers();
};

} // namespace hrd41

#endif // __ILRD_INPUT_MEDIATOR_HPP__