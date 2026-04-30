# Observer Pattern (`/design_patterns/observer/`)

## Pattern Overview

**Purpose**: Establish a one-to-many dependency where when one object changes state, all its dependents are notified automatically.

**Problem It Solves**: 
- How to decouple event producers from event consumers?
- How to allow multiple observers to react to the same event?
- How to avoid tight coupling between components?

**Real-World Analogy**: Like a news magazine - readers (observers) subscribe to a magazine (subject). When a new issue is published, all subscribers are automatically notified.

## Why Use Observer Pattern?

✅ **Loose Coupling**: Objects don't need to know about each other
✅ **Scalability**: Easy to add new observers without changing producer
✅ **Flexibility**: Runtime subscription/unsubscription
✅ **Event-Driven Design**: Natural fit for reactive systems
✅ **Separation of Concerns**: Producers and consumers are independent

## Implementation Structure

```
design_patterns/observer/
├─ include/              # Headers
│  ├─ Dispatcher.hpp     # Event broadcaster
│  ├─ ICallBack.hpp      # Observer interface
│  └─ CallBack.hpp       # Concrete observer
└─ src/                  # Implementation
   └─ (implementation in headers - templates)
```

## Core Components

### 1. Dispatcher (Event Broadcaster)

```cpp
template <typename T>
class Dispatcher {
public:
    void Register(ICallBack<T>* observer);
    void UnRegister(ICallBack<T>* observer);
    void NotifyAll(const T& msg);
};
```

**Responsibility**: Maintains list of observers and broadcasts events

**Key Features**:
- Template-based for type safety
- Automatic cleanup on destruction
- Thread-safe registration/unregistration

---

### 2. ICallBack (Observer Interface)

```cpp
template <typename T>
class ICallBack {
public:
    virtual ~ICallBack() = default;
    virtual void update(const T& msg) = 0;
};
```

**Responsibility**: Define the contract for all observers

**Key Features**:
- Pure virtual interface
- Template parameter for event type
- Clean abstract design

---

### 3. CallBack (Concrete Observer)

```cpp
template <typename T, typename Sub>
class CallBack : public ICallBack<T> {
public:
    CallBack(Dispatcher<T>* dispatcher,
             Sub* subscriber,
             void (Sub::*handler)(const T&));
    
    void update(const T& msg) override;
};
```

**Responsibility**: Implement observer and call handler method

**Key Features**:
- Auto-registers with dispatcher
- Uses member function pointers
- Auto-unregisters on destruction

## Usage Example

### Scenario: File System Monitoring

```cpp
#include "Dispatcher.hpp"
#include "CallBack.hpp"

// Define event type
struct FileEvent {
    std::string filename;
    std::string action;  // "CREATE", "DELETE", "MODIFY"
};

// Create dispatcher
Dispatcher<FileEvent> file_dispatcher;

// Define observer class
class Logger {
public:
    void onFileEvent(const FileEvent& event) {
        std::cout << "📝 [Logger] " << event.action 
                  << ": " << event.filename << std::endl;
    }
};

// Create observer instance
Logger logger;

// Register observer (auto-registers in constructor)
CallBack<FileEvent, Logger> observer(&file_dispatcher, 
                                     &logger, 
                                     &Logger::onFileEvent);

// Notify all observers
FileEvent event{"test.txt", "CREATE"};
file_dispatcher.NotifyAll(event);

// Output: 📝 [Logger] CREATE: test.txt
// Observer auto-unregisters when destroyed
```

## Used In Local Cloud Project

### Phase 1: Dynamic Plugin Loading

**DirMonitor → Dispatcher → PNP**

```
┌──────────────────┐
│  DirMonitor      │  Detects filesystem events
└────────┬─────────┘
         │ NotifyAll(FileEvent)
         ↓
┌──────────────────┐
│  Dispatcher      │  Broadcasts to all observers
└────────┬─────────┘
         │
    ┌────┴────┐
    ↓         ↓
┌───────┐ ┌──────┐
│  PNP  │ │Other │
└───────┘ │Obs.  │
          └──────┘
```

### Code in Project

```cpp
// DirMonitor uses Dispatcher to notify PNP
Dispatcher<DirEvent> dispatcher;
CallBack<DirEvent, PNP> pnp_observer(&dispatcher,
                                     &pnp,
                                     &PNP::onDirChange);

// File appears
dispatcher.NotifyAll(DirEvent{FILE_CREATED, "plugin.so"});

// PNP::onDirChange() is called automatically
```

## Pattern Flow Diagram

```
1. Observer Registration
   Observer → CallBack constructor → Dispatcher::Register()

2. Event Notification
   Subject → Dispatcher::NotifyAll(event)
          ↓
       For each observer:
          → ICallBack::update(event)
             → Sub::handler(event)

3. Observer Unregistration
   Observer destroyed → CallBack destructor → Dispatcher::UnRegister()
```

## Key Design Decisions

### ✅ Why Template-Based?

```cpp
// Type-safe: compiler catches wrong event types
Dispatcher<FileEvent> file_dispatcher;
Dispatcher<PluginEvent> plugin_dispatcher;

// Can't accidentally mix events
// file_dispatcher.NotifyAll(plugin_event);  // Compiler error!
```

### ✅ Why Member Function Pointers?

```cpp
// Allows binding to any class method
CallBack<Event, MyClass> observer1(&disp, &obj1, &MyClass::handler1);
CallBack<Event, OtherClass> observer2(&disp, &obj2, &OtherClass::handler2);

// Flexible: each observer has its own class type
```

### ✅ Why Auto-Registration/Unregistration?

```cpp
// RAII principle: resource cleanup is automatic
{
    CallBack<Event, Observer> obs(&dispatcher, this, &Observer::update);
    // Observer is registered
}  // Destructor called - observer automatically unregistered
   // No memory leaks, no forgotten unregistration
```

## Comparison with Alternatives

### vs. Direct Function Calls
```cpp
// ❌ Direct: Tight coupling
file_monitor.addListener(&logger);  // Monitor knows about Logger

// ✅ Observer: Loose coupling
dispatcher.NotifyAll(event);        // Monitor doesn't know listeners exist
```

### vs. Global Event Bus
```cpp
// ❌ Global: Hard to test, hidden dependencies
GlobalEventBus::notify(event);      // Who's listening?

// ✅ Observer: Clear dependencies
Dispatcher<Event> local_dispatcher;  // Explicit, testable
```

### vs. Callbacks (Raw Function Pointers)
```cpp
// ❌ Raw callbacks: No type safety
dispatcher.subscribe((void*)&function_pointer);

// ✅ Observer: Type-safe, object-oriented
CallBack<Event, MyClass> observer(&dispatcher, this, &MyClass::method);
```

## Threading Considerations

### Current Implementation
- Uses RAII for thread-safe registration/unregistration
- NotifyAll is NOT thread-safe during iteration

### For Thread-Safe Scenarios
```cpp
// Copy observer list before notifying
std::vector<ICallBack<T>*> observers_copy;
{
    std::lock_guard<std::mutex> lock(mutex);
    observers_copy = observers;
}
for (auto obs : observers_copy) {
    obs->update(msg);
}
```

## Common Pitfalls

### ❌ Forgetting to Unregister
```cpp
// Bad: Observer not unregistered
new CallBack<Event, Logger>(&dispatcher, &logger, &Logger::onEvent);
// Observer is leaked, keeps receiving events
```

### ✅ Proper RAII Usage
```cpp
// Good: Observer unregisters automatically
CallBack<Event, Logger> observer(&dispatcher, &logger, &Logger::onEvent);
// Observer unregisters when going out of scope
```

### ❌ Circular Dependencies
```cpp
// Bad: A notifies B, B notifies A
Dispatcher<EventA> disp_a;
Dispatcher<EventB> disp_b;
// A observes B, B observes A → potential infinite loops
```

### ✅ Proper Architecture
```cpp
// Good: Clear dependency hierarchy
Events flow: FileSystem → DirMonitor → Dispatcher → PNP
// No cycles, unidirectional data flow
```

## Testing Observer Pattern

### Run Test
```bash
./bin/test_dispatcher
# Tests:
# - Registration/unregistration
# - Event notification
# - Multiple observers
# - RAII cleanup
```

### Example Test
```cpp
#include "Dispatcher.hpp"
#include "CallBack.hpp"

struct TestEvent {
    int value;
};

class TestObserver {
public:
    int received = 0;
    void onEvent(const TestEvent& evt) {
        received = evt.value;
    }
};

int main() {
    Dispatcher<TestEvent> dispatcher;
    TestObserver observer;
    
    {
        CallBack<TestEvent, TestObserver> cb(&dispatcher, 
                                             &observer,
                                             &TestObserver::onEvent);
        
        TestEvent evt{42};
        dispatcher.NotifyAll(evt);
        assert(observer.received == 42);  // ✅ Observer notified
    }
    // CallBack destroyed, unregistered
    
    TestEvent evt2{99};
    dispatcher.NotifyAll(evt2);
    assert(observer.received == 42);  // ✅ Observer NOT notified (unregistered)
    
    return 0;
}
```

## Related Patterns

### Observer + Factory
```
Factory creates plugins
    ↓
Plugin subscribes to events
    ↓
Dispatcher notifies plugin on events
```

### Observer + Singleton
```
Singleton dispatcher instance
    ↓
Multiple observers register
    ↓
Global event distribution
```

### Observer + Command
```
Event received by Dispatcher
    ↓
Create Command from event
    ↓
Execute Command in ThreadPool
```

## Extension Points

### Filtering Events
```cpp
template <typename T>
class FilteredCallBack : public ICallBack<T> {
    virtual bool shouldUpdate(const T& msg) { return true; }
};
```

### Priority Observers
```cpp
Dispatcher dispatcher;
dispatcher.Register(high_priority_obs, PRIORITY_HIGH);
dispatcher.Register(low_priority_obs, PRIORITY_LOW);
```

### Async Event Handling
```cpp
dispatcher.NotifyAllAsync([](ICallBack<T>* obs, const T& msg) {
    threadpool.enqueue([obs, msg] { obs->update(msg); });
});
```

## Summary

| Aspect | Observer Pattern |
|--------|------------------|
| **Use When** | Need loose coupling, event-driven architecture |
| **Avoids** | Tight coupling, hard-to-extend systems |
| **Key Benefit** | Scalable, flexible, maintainable |
| **Complexity** | Low - simple but powerful |
| **Thread-Safe** | Mostly, with caveats for iteration |

---

**Location**: `design_patterns/observer/`  
**Status**: ✅ Fully implemented and tested  
**Used By**: DirMonitor, PNP, all event-driven components
