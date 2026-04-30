// ============================================================================
// ICallBack.hpp
// What:  Observer pattern - base class for observers that receive messages
// Why:   Allows multiple observers to register with a Dispatcher and receive
//        notifications when events occur
// How:   Subclass ICallBack, pass Dispatcher* to constructor (auto-registers),
//        override Notify(msg) to handle events
// ============================================================================

#ifndef __ILRD_ICALLBACK_HPP__
#define __ILRD_ICALLBACK_HPP__

#include "logger.hpp"
#include "singelton.hpp"

namespace hrd41
{

template <typename Msg> class Dispatcher;

template <typename Msg> class ICallBack
{
public:
  explicit ICallBack(Dispatcher<Msg>* disp);
  ICallBack(const ICallBack& o) = delete;
  ICallBack& operator=(const ICallBack& o) = delete;
  virtual ~ICallBack() = 0;

  virtual void Notify(const Msg& msg) = 0;
  virtual void NotifyEnd() = 0;

private:
  Dispatcher<Msg>* m_disp;
  Logger* m_logger;
};
template <typename Msg>
ICallBack<Msg>::ICallBack(Dispatcher<Msg>* disp_)
    : m_disp(disp_), m_logger(Singleton<Logger>::GetInstance())
{
  if (m_disp)
  {
    m_disp->Register(this);
  }
}

template <typename Msg> ICallBack<Msg>::~ICallBack()
{
  if (m_disp)
  {
    m_disp->UnRegister(this);
    m_disp = nullptr;
  }
}

} // namespace hrd41
#endif