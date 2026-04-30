# Design Patterns (`/design_patterns`)

## Purpose

This directory contains **design pattern implementations** - reusable solutions to common programming problems. Each pattern is used throughout the Local Cloud infrastructure to solve specific architectural challenges.

## What Are Design Patterns?

Design patterns are proven, reusable templates for solving recurring problems in software design. They:
- Provide common vocabulary for communicating ideas
- Speed up development by providing solutions
- Make code more maintainable and understandable
- Help avoid common pitfalls

## Patterns Implemented

### 1. **Observer Pattern** 📡

**Location**: [`./observer/`](./observer/README.md)

**Problem**: How do components notify each other about events without tight coupling?

**Solution**: Implement a dispatcher that publishes events to registered observers.

**Used in**:
- DirMonitor → PNP (filesystem events)
- Dispatcher → Callbacks (async notifications)

**Example**:
```cpp
Dispatcher<FileEvent> dispatcher;
CallBack<FileEvent, MyClass> observer(&dispatcher, &MyClass::onFileEvent);
dispatcher.NotifyAll(event); // All observers notified
```

---

### 2. **Factory Pattern** 🏭

**Location**: [`./factory/`](./factory/README.md)

**Problem**: How to create objects without hardcoding their types?

**Solution**: Use a factory to encapsulate object creation.

**Used in**:
- Plugin registration and instantiation
- Service creation (Phase 2)
- Command object creation

**Example**:
```cpp
PluginFactory::getInstance().registerPlugin("MyPlugin", creator);
MyPlugin* plugin = PluginFactory::getInstance().create("MyPlugin");
```

---

### 3. **Singleton Pattern** 🔒

**Location**: [`./singleton/`](./singleton/README.md)

**Problem**: How to ensure only one instance of a class exists globally?

**Solution**: Implement a class that creates only one instance and provides global access.

**Used in**:
- Logger (single log output)
- PluginFactory (single registry)
- ServiceRegistry (Phase 2)

**Example**:
```cpp
Logger& logger = Logger::getInstance();
logger.log("Message");
```

---

### 4. **Command Pattern** ⚙️

**Location**: [`./command/`](./command/README.md)

**Problem**: How to encapsulate requests as objects for queuing and undo/redo?

**Solution**: Represent commands as objects with execute() method.

**Used in**:
- Task execution in thread pool
- RPC method invocation (Phase 3)
- Plugin command dispatch

**Example**:
```cpp
ICommand<Args, Result>* cmd = new MyCommand(args);
Result result = cmd->execute();
```

---

## Directory Structure

```
design_patterns/
├─ README.md              # This file
├─ observer/              # Observer pattern (Dispatcher + CallBack)
├─ factory/               # Factory pattern (Object creation)
├─ singleton/             # Singleton pattern (Global instances)
└─ command/               # Command pattern (Request encapsulation)
```

## Pattern Relationships

```
Singleton Pattern
    ↓ provides global access to
┌─────────────────────────┐
│ Factory Pattern         │
├─────────────────────────┤
│ PluginFactory           │
│ ServiceRegistry (Phase 2)
└─────────────────────────┘
    ↑ creates instances for
Command Pattern
    ↓ encapsulates
┌─────────────────────────┐
│ Observer Pattern        │
├─────────────────────────┤
│ Dispatcher              │
│ CallBack                │
└─────────────────────────┘
    ↓ notifies observers
Event-driven Architecture
```

## Usage Throughout the Project

### Phase 1: Dynamic Plugin Loading ✅

```
Plugin Loading Flow
    ↓
Loader (uses Factory to create plugins)
    ↓
DirMonitor (observes filesystem)
    ↓
Dispatcher (Observer pattern)
    ↓
PNP (orchestrates loading)
    ↓
PluginFactory (Singleton + Factory)
```

### Phase 2: Service Registry (Planned)

```
Service Registration
    ↓
ServiceRegistry (Singleton factory)
    ↓
Service Discovery
    ↓
Health Check Dispatcher (Observer pattern)
```

### Phase 3: RPC & Messaging (Planned)

```
Message Receiving
    ↓
Command Pattern (deserialize to commands)
    ↓
Execute (Command::execute())
    ↓
Dispatcher (notify response observers)
```

## Pattern Selection Guide

| Problem | Solution | Pattern |
|---------|----------|---------|
| Need global access to service | One instance everywhere | Singleton |
| Need to notify many listeners | Event distribution | Observer |
| Need type-safe object creation | Abstract factory | Factory |
| Need to queue and execute work | Encapsulate as object | Command |
| Need plugin extensibility | Load at runtime | Factory |
| Need component independence | Event-based communication | Observer |

## When to Use Each Pattern

### Use Observer When:
- ✅ Multiple components need to react to an event
- ✅ Components should be loosely coupled
- ✅ You need one-to-many communication
- ✅ You want event-driven architecture

### Use Factory When:
- ✅ Object creation is complex
- ✅ You want to hide creation logic
- ✅ Plugins need to self-register
- ✅ You want interchangeable implementations

### Use Singleton When:
- ✅ Exactly one instance must exist
- ✅ You need global access
- ✅ Shared state must be centralized
- ✅ Resource should be lazily initialized

### Use Command When:
- ✅ You need to queue operations
- ✅ You want to parameterize methods
- ✅ You need to support undo/redo
- ✅ You want to decouple invoker from executor

## Anti-Patterns to Avoid

### ❌ Don't Use Singleton for Everything
- Creates hard-to-test code
- Hides dependencies
- Makes concurrency harder

### ❌ Don't Use Observer Without Clear Event Types
- Unclear what events mean
- Hard to debug event flows
- Memory leaks from forgotten unregistration

### ❌ Don't Ignore Factory Complexity
- Over-engineering simple cases
- Too many factory patterns
- Unused abstraction layers

### ❌ Don't Use Command Pattern for Simple Cases
- Adds unnecessary complexity
- Simple function calls are fine
- Reserve for complex queueing needs

## Best Practices

### 1. **Observer Pattern**
- Use strongly-typed events: `Dispatcher<Event>`
- Always unregister observers: use RAII
- Document what events mean
- Use const references: `const Event&`

### 2. **Factory Pattern**
- Keep factory interface simple
- Document object lifetime ownership
- Use unique_ptr for memory safety
- Validate created objects

### 3. **Singleton Pattern**
- Use lazy initialization
- Thread-safe access
- Avoid in library code
- Document threading model

### 4. **Command Pattern**
- Template for type safety: `ICommand<Args, Return>`
- Execute should be idempotent
- Store parameters, not state
- Document command semantics

## Pattern Implementation Examples

All patterns are fully implemented and tested:

```bash
# Test Observer Pattern
./bin/test_dispatcher

# Test Factory Pattern
./bin/test_plugin_load    # Plugins use factory

# Test Singleton Pattern
./bin/test_singelton

# Test Command Pattern
./bin/test_command_demo
```

## Integration Architecture

```
┌─────────────────────────────────────────────┐
│       Application Layer                     │
│  (Uses patterns for functionality)          │
└─────────────────────────────────────────────┘
            ↑
┌─────────────────────────────────────────────┐
│      Design Patterns Layer                  │
│  ├─ Observer (event distribution)          │
│  ├─ Factory (object creation)              │
│  ├─ Singleton (global state)               │
│  └─ Command (request encapsulation)        │
└─────────────────────────────────────────────┘
            ↑
┌─────────────────────────────────────────────┐
│      Core Components                        │
│  ├─ Dispatcher, CallBack (Observer)        │
│  ├─ PluginFactory (Factory)                │
│  ├─ Logger, ServiceRegistry (Singleton)    │
│  └─ ThreadPool tasks (Command)             │
└─────────────────────────────────────────────┘
```

## Learning Resources

For each pattern, see:
- **README.md** - Overview and motivation
- **include/** - Header definitions
- **src/** - Implementation details
- **test/** - Real-world examples

## Pattern Progression Through Phases

| Phase | Patterns Used |
|-------|---------------|
| 1 | Observer, Factory, Singleton, RAII |
| 2 | + Registry, Service locator |
| 3 | + Strategy (protocol selection) |
| 4 | + Adapter (interface conversion) |
| 5 | + Bridge (storage abstraction) |
| 6 | + Proxy (security wrapper) |

## Next Steps

### To Understand a Pattern
1. Read the pattern's README.md
2. Study the implementation
3. Run the test binaries
4. Try modifying the test code

### To Learn All Patterns
Read in order:
1. Singleton (simplest)
2. Factory (builds on singleton)
3. Observer (event-driven)
4. Command (most complex)

---

**Status**: ✅ All core patterns implemented and tested  
**Phase**: 1 (Dynamic Plugin Loading) + foundation for all future phases
