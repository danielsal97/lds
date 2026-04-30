// ============================================================================
// CallBack.hpp
// What:  Concrete observer that binds Dispatcher notifications to object method
// Why:   Connects Dispatcher to subscriber objects; wraps member function
//        pointers so any object can receive notifications
// How:   CallBack(dispatcher, subscriber, actionFunc, stopFunc) binds
//        subscriber's methods; Notify() calls action method with message
// ============================================================================

#ifndef __ILRD_CALLBACK_HPP__
#define __ILRD_CALLBACK_HPP__

#include "Dispatcher.hpp"
#include "ICallBack.hpp"

namespace hrd41
{

template <typename Msg, typename Sub> class CallBack : public ICallBack<Msg>
{
public:
  using ActionFunc = void (Sub::*)(const Msg&);
  using StopFunc = void (Sub::*)();

  explicit CallBack(Dispatcher<Msg>* disp, Sub& sub, ActionFunc actFunc,
                    StopFunc stopFunc = nullptr);

  CallBack(const CallBack& o) = delete;
  CallBack& operator=(const CallBack& o) = delete;
  ~CallBack();

  void Notify(const Msg& msg) override;
  void NotifyEnd() override;

private:
  Dispatcher<Msg>* m_disp;
  Sub& m_sub;
  ActionFunc m_actFunc;
  StopFunc m_stopfunc;
};

template <typename Msg, typename Sub>
CallBack<Msg, Sub>::CallBack(Dispatcher<Msg>* disp_, Sub& sub_,
                             ActionFunc actFunc_, StopFunc stopFunc_)
    : ICallBack<Msg>(disp_), m_disp(disp_), m_sub(sub_), m_actFunc(actFunc_),
      m_stopfunc(stopFunc_)
{
}

template <typename Msg, typename Sub> CallBack<Msg, Sub>::~CallBack()
{
  m_disp->UnRegister(this);
}
template <typename Msg, typename Sub>
void CallBack<Msg, Sub>::Notify(const Msg& Msg_)
{
  (m_sub.*m_actFunc)(Msg_);
}

template <typename Msg, typename Sub> void CallBack<Msg, Sub>::NotifyEnd()
{
  if (m_stopfunc != nullptr)
  {
    (m_sub.*m_stopfunc)();
  }
}

} // namespace hrd41

#endif // __ILRD_CALLBACK_HPP__