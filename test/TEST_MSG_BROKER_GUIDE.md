# Message Broker Test Guide

## Overview
The message broker implements a **pub-sub (Observer) pattern** where:
- **Dispatcher** acts as a publisher that sends messages to all registered subscribers
- **ICallBack** is an abstract base class for subscribers
- **CallBack** is a concrete implementation that wraps member function calls

## How It Works

### 1. Basic Flow
```
Dispatcher -> NotifyAll(message) -> calls notify() on all registered subscribers
```

### 2. Registration
Subscribers register themselves with the Dispatcher when they are constructed:
```cpp
IntSubscriber sub(&dispatcher);  // Constructor calls Register()
```

### 3. Unregistration
Subscribers unregister themselves when destroyed:
```cpp
dispatcher.UnRegister(&sub);  // Manual unregistration
// or automatically in destructor
```

---

## Test Cases Explained

### Test 1: `test_basic_notification()`
**What it tests:** Single message sent to one subscriber
- Create a dispatcher and subscriber
- Send message (42)
- Verify subscriber received exactly 1 message with value 42

### Test 2: `test_multiple_notifications()`
**What it tests:** Multiple messages to the same subscriber
- Send 3 different messages (10, 20, 30)
- Verify count is 3
- Verify last message received was 30

### Test 3: `test_multiple_subscribers()`
**What it tests:** One message broadcasts to multiple subscribers
- Create 3 subscribers
- Send 1 message
- All 3 should receive the same message

### Test 4: `test_unregister()`
**What it tests:** Removing a subscriber from the notification list
- Create 2 subscribers, both receive message 1
- Unregister subscriber 1
- Send message 2 - only subscriber 2 receives it
- subscriber 1 count stays at 1, subscriber 2 count becomes 2

### Test 5: `test_callback_with_member_functions()`
**What it tests:** Using CallBack to wrap member function calls
- Create a MessageHandler object
- Create CallBack that wraps its `handleMessage()` method
- Send message through dispatcher
- Verify handler received it

### Test 6: `test_callback_without_stop_func()`
**What it tests:** CallBack with optional stop function (pass nullptr)
- Stop function is optional
- Works correctly even when not provided

### Test 7: `test_notify_end()`
**What it tests:** NotifyEnd callback (cleanup/shutdown notification)
- Subscribers can implement notifyEnd() for cleanup
- Called when dispatcher/subscriber is destroyed

### Test 8: `test_string_messages()`
**What it tests:** Message broker works with different types
- Uses `std::string` instead of `int`
- Dispatcher is template-based and works with any type

### Test 9: `test_struct_messages()`
**What it tests:** Complex custom types
- Uses a struct with multiple fields
- Verifies all data is transmitted correctly

---

## Key Concepts

### Template-Based Design
```cpp
Dispatcher<int>        // Sends int messages
Dispatcher<std::string> // Sends string messages
CallBack<int, MyHandler> // int messages, MyHandler as subscriber
```

### Member Function Pointers
```cpp
CallBack<int, MessageHandler> callback(
    &dispatcher,
    handler,
    &MessageHandler::handleMessage,  // Member function pointer
    &MessageHandler::stop            // Optional stop function
);
```

### Automatic Registration/Unregistration
When you create an ICallBack subclass, it automatically registers itself:
```cpp
IntSubscriber sub(&dispatcher);  // Constructor registers
// sub is destroyed here -> destructor unregisters
```

---

## To Run the Tests

```bash
make test_msg_broker    # Compile the test
bin/test_msg_broker     # Run the test binary
```

Or compile and run all tests:
```bash
make run_tests
```

---

## Expected Output

You should see output like:
```
[PASS] basic_notification_received
[PASS] basic_notification_correct_message
[PASS] multiple_notifications_count
...
```

All tests should PASS if the implementation is correct.
