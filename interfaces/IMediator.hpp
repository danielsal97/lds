// ============================================================================
// IMediator.hpp
// What:  Interface for mediator pattern (Reactor ↔ RAID Manager coordinator)
// Why:   Abstracts mediator implementation; allows swapping different
//        mediator strategies without changing Reactor
// How:   Reactor calls Notify(fd) when epoll detects event; mediator
//        receives request, dispatches to RAID Manager, sends response
// ============================================================================

#ifndef __ILRD_IMEDITOR_HPP__
#define __ILRD_IMEDITOR_HPP__

namespace hrd41
{

class IMediator
{
public:
  explicit IMediator() = default;
  IMediator(const IMediator& o_) = delete;
  IMediator& operator=(const IMediator& o_) = delete;
  virtual ~IMediator() = default;

  virtual void Notify(int fd) = 0;
};

} // namespace hrd41

#endif // __ILRD_IMEDITOR_HPP__
