# Message Broker - Expected Behavior (TDD)

This document describes what behavior the tests expect. Use these tests to guide your implementation.

## Architecture Overview

```
┌─────────────────────────────────────────────────┐
│           Dispatcher<Msg>                       │
│  - Maintains list of subscribers                │
│  - Sends messages to all subscribers            │
└─────────────────────────────────────────────────┘
         ▲                    ▼
         │              NotifyAll(msg)
         │                    │
         │                    ├─► notify(msg) to subscriber 1
         │                    ├─► notify(msg) to subscriber 2
         │                    └─► notify(msg) to subscriber 3
         │
    Register()
    UnRegister()
         │
┌─────────────────────────────────────────────────┐
│        ICallBack<Msg> / CallBack<Msg, Sub>      │
│  - Implements notify(msg)                       │
│  - Implements notifyEnd()                       │
└─────────────────────────────────────────────────┘
```

---

## Test 1: Subscriber Registers on Creation

**Code:**
```cpp
Dispatcher<int> dispatcher;
SimpleSubscriber sub(&dispatcher);
dispatcher.NotifyAll(42);
```

**Expected Behavior:**
- When `SimpleSubscriber` is constructed with a `Dispatcher*`, it **automatically registers** itself with the dispatcher
- When `NotifyAll()` is called, the dispatcher should **call `notify(msg)` on all registered subscribers**
- The subscriber should receive the message and update its internal state

**What needs to be implemented:**
- `Dispatcher` constructor must initialize an empty subscriber list
- `ICallBack` constructor must call `Register()` on the dispatcher
- `Dispatcher::Register()` must add the subscriber to the list
- `Dispatcher::NotifyAll()` must iterate through all subscribers and call `notify(msg)` on each

---

## Test 2: Multiple Messages to One Subscriber

**Code:**
```cpp
dispatcher.NotifyAll(10);
dispatcher.NotifyAll(20);
dispatcher.NotifyAll(30);
```

**Expected Behavior:**
- The same subscriber receives all messages in order
- The subscriber's count increments (1 → 2 → 3)
- Only the last message (30) is stored in `m_last_value`

**What needs to be implemented:**
- `Dispatcher::NotifyAll()` is idempotent - can be called multiple times
- Each call sends to all current subscribers

---

## Test 3: One Message to Multiple Subscribers

**Code:**
```cpp
SimpleSubscriber sub1(&dispatcher);
SimpleSubscriber sub2(&dispatcher);
SimpleSubscriber sub3(&dispatcher);
dispatcher.NotifyAll(100);
```

**Expected Behavior:**
- All three subscribers are registered (in order)
- When one message is sent, **all three receive it**
- All have count = 1 and lastValue = 100

**What needs to be implemented:**
- `Dispatcher` must maintain a list (vector) of subscribers
- `NotifyAll()` must iterate through **all** subscribers in the list
- Multiple subscribers can exist simultaneously

---

## Test 4: Unregister Removes Subscriber

**Code:**
```cpp
dispatcher.NotifyAll(10);  // Both get message 1
dispatcher.UnRegister(&sub1);
dispatcher.NotifyAll(20);  // Only sub2 gets message 2
```

**Expected Behavior:**
- Sub1: count = 1, lastValue = 10 (unregistered, no more messages)
- Sub2: count = 2, lastValue = 20 (still registered)

**What needs to be implemented:**
- `Dispatcher::UnRegister()` must find and remove the subscriber from the list
- After unregistration, that subscriber receives no more messages
- Other subscribers are unaffected

---

## Test 5: CallBack with Member Function

**Code:**
```cpp
MyHandler handler;
CallBack<int, MyHandler> callback(
    &dispatcher,
    handler,
    &MyHandler::handleMessage,
    &MyHandler::onStop
);
dispatcher.NotifyAll(55);
```

**Expected Behavior:**
- `CallBack` is a concrete implementation of `ICallBack`
- It holds a reference to the handler object
- When `notify(msg)` is called on the callback, it calls `handler.handleMessage(msg)`
- The handler receives the message through its member function

**What needs to be implemented:**
- `CallBack` constructor must store:
  - The dispatcher pointer
  - A reference to the subscriber object
  - Member function pointers to the action and stop functions
- `CallBack::notify(msg)` must call the stored member function on the subscriber:
  ```cpp
  (m_sub.*m_handleFunc)(msg);
  ```

---

## Test 6: CallBack Without Stop Function

**Code:**
```cpp
CallBack<int, MyHandler> callback(
    &dispatcher,
    handler,
    &MyHandler::handleMessage,
    nullptr  // Optional stop function
);
```

**Expected Behavior:**
- Stop function is optional (can be nullptr)
- CallBack still works correctly for receiving messages
- When `notifyEnd()` is called, it should safely handle the nullptr

**What needs to be implemented:**
- `CallBack::notifyEnd()` must check if `m_stopfunc` is not nullptr before calling it:
  ```cpp
  if (m_stopfunc != nullptr) {
      (m_sub.*m_stopfunc)();
  }
  ```

---

## Test 7: NotifyEnd Signal

**Code:**
```cpp
CallBack<int, MyHandler> callback(
    &dispatcher,
    handler,
    &MyHandler::handleMessage,
    &MyHandler::onStop
);
dispatcher.NotifyAll(111);
// When callback is destroyed, notifyEnd() should be called
```

**Expected Behavior:**
- There should be a way to signal subscribers that messages have ended
- `notifyEnd()` is called for cleanup/shutdown notification

**What needs to be implemented:**
- `Dispatcher` should have a method to notify end (or it happens in destructor)
- `CallBack::notifyEnd()` calls the stored stop function:
  ```cpp
  if (m_stopfunc != nullptr) {
      (m_sub.*m_stopfunc)();
  }
  ```

---

## Key Implementation Details

### Member Function Pointers

The pattern used:
```cpp
using ActionFunc = void(Sub::*)(const Msg&);  // Pointer to member function

// Stored as:
ActionFunc m_handleFunc;

// Called as:
(m_sub.*m_handleFunc)(msg);  // Dereference and call through pointer
```

### Auto-Registration Pattern

```cpp
// In ICallBack constructor:
explicit ICallBack(Dispatcher<Msg>* disp) {
    m_disp = disp;
    m_disp->Register(this);  // Register self
}

// In destructor:
~ICallBack() {
    m_disp->UnRegister(this);  // Unregister self
}
```

### Dispatcher Implementation

```cpp
// Storage
std::vector<ICallBack<Msg>*> m_subs;

// Registration
void Register(ICallBack<Msg>* sub) {
    m_subs.push_back(sub);
}

// Unregistration
void UnRegister(ICallBack<Msg>* sub) {
    // Remove sub from m_subs vector
}

// Notification
void NotifyAll(const Msg& msg) {
    for (auto sub : m_subs) {
        sub->notify(msg);
    }
}
```

---

## Run the Tests

Once you implement the classes, run:
```bash
make test_msg_broker
bin/test_msg_broker
```

All tests should PASS when the implementation is correct.
