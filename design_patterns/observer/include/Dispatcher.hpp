// ============================================================================
// Dispatcher.hpp
// What:  Observer pattern - broadcasts messages to all registered observers
// Why:   Central hub for event notifications; decouples event sources from
//        event handlers (e.g., filesystem events → multiple handlers)
// How:   Observers Register()/UnRegister() themselves; NotifyAll(msg) sends
//        message to all registered observers
// ============================================================================

#ifndef __ILRD_DISPATCHER_H
#define __ILRD_DISPATCHER_H

#include "ICallBack.hpp"
#include "logger.hpp"
#include "singelton.hpp"
#include <string>
#include <vector>

namespace hrd41
{
template <typename Msg> class Dispatcher
{
public:
  explicit Dispatcher();
  Dispatcher(const Dispatcher& o) = delete;
  Dispatcher& operator=(const Dispatcher& o) = delete;
  ~Dispatcher();

  void Register(ICallBack<Msg>* sub);
  void UnRegister(ICallBack<Msg>* sub);
  void NotifyAll(const Msg& msg);

private:
  std::vector<ICallBack<Msg>*> m_subs;
  Logger* m_logger;
};
template <typename Msg>
Dispatcher<Msg>::Dispatcher() : m_logger(Singleton<Logger>::GetInstance())
{
  m_logger->SetLoggerLevel(Logger::DEBUG);
  m_logger->Write("Ctor: Dispathcer", Logger::INFO);
}

template <typename Msg> Dispatcher<Msg>::~Dispatcher()
{

  m_logger->Write("Dtor: Dispathcer", Logger::INFO);
  for (ICallBack<Msg>* sub : m_subs)
  {
    if (sub)
    {
      sub->NotifyEnd();
    }
  }
}

template <typename Msg> void Dispatcher<Msg>::Register(ICallBack<Msg>* sub_)
{
  m_subs.push_back(sub_);
  m_logger->Write(
      ("Adding sub \n sub is now : " + std::to_string(m_subs.size())),
      Logger::INFO);
}

template <typename Msg> void Dispatcher<Msg>::UnRegister(ICallBack<Msg>* sub_)
{
  m_subs.erase(std::remove(m_subs.begin(), m_subs.end(), sub_), m_subs.end());
}

template <typename Msg> void Dispatcher<Msg>::NotifyAll(const Msg& msg_)
{
  for (ICallBack<Msg>* sub : m_subs)
  {
    sub->Notify(msg_);
  }
}
} // namespace hrd41
#endif