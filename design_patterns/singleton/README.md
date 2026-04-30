# Singleton Pattern (`/design_patterns/singleton/`)

## Pattern Overview

**Purpose**: Ensure a class has only one instance and provide a global point of access to it.

**Problem It Solves**:
- How to ensure only one instance of a class exists?
- How to provide global access without global variables?
- How to lazy-initialize expensive resources?
- How to manage shared state safely?

**Real-World Analogy**: Like a company CEO - there's only one CEO, and you contact them through a specific channel.

## Why Use Singleton Pattern?

✅ **Controlled Access**: Only one instance, guaranteed
✅ **Global Access**: Available everywhere in the application
✅ **Lazy Initialization**: Created only when first needed
✅ **Resource Protection**: Expensive resources created once
✅ **Thread-Safe**: Synchronized access in multi-threaded environment
✅ **Simple**: Easy to understand and use

## Implementation Structure

```
design_patterns/singleton/
├─ include/              # Headers
│  └─ singelton.hpp      # Singleton template
└─ src/                  # Implementation
   └─ (implementation in headers - templates)
```

## Core Component

### Singleton Template

```cpp
template <typename T>
class Singleton {
public:
    static T& getInstance() {
        // Thread-safe: local static initialization (C++11)
        static T instance;
        return instance;
    }
    
private:
    Singleton() = default;
    ~Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
};
```

**Key Features**:
- Thread-safe (C++11 magic statics)
- Lazy initialization (first call)
- No copying allowed
- Simple, single pattern

---

## Usage Example

### Scenario: Global Logger

```cpp
#include "singelton.hpp"
#include <iostream>
#include <fstream>

// Logger class
class Logger {
public:
    void log(const std::string& message) {
        std::cout << "[LOG] " << message << std::endl;
    }
    
    void setLevel(int level) {
        this->level = level;
    }

private:
    int level = 0;
    
    // Make constructor private - only getInstance can create
    Logger() { std::cout << "Logger initialized" << std::endl; }
    friend class Singleton<Logger>;
    
    // Prevent copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

// Usage: Access logger from anywhere
int main() {
    // First call: creates instance
    Logger& logger1 = Singleton<Logger>::getInstance();
    logger1.log("Starting application");
    
    // Second call: returns same instance
    Logger& logger2 = Singleton<Logger>::getInstance();
    logger2.setLevel(1);
    
    // Both are the same object
    assert(&logger1 == &logger2);  // ✅ Same instance
    
    return 0;
}
```

## Used In Local Cloud Project

### Examples in Codebase

```cpp
// Logger (global logging)
Logger& logger = Logger::getInstance();
logger.log("Plugin loaded");

// PluginFactory (plugin registry)
auto& factory = PluginFactory::getInstance();
factory.registerType("MyPlugin", creator);

// ServiceRegistry (Phase 2 - planned)
auto& registry = ServiceRegistry::getInstance();
registry.registerService("database", service);
```

### Singleton Hierarchy

```
Singleton<Logger>
    └─ One global logger instance
       ├─ DirMonitor logs events
       ├─ Loader logs plugin loading
       ├─ PNP logs orchestration
       └─ Plugins log their operations

Singleton<PluginFactory>
    └─ One global plugin registry
       ├─ Plugins register on load
       ├─ Applications query registry
       └─ PNP manages plugins

Singleton<ServiceRegistry>  (Phase 2)
    └─ One global service registry
       ├─ Services register on startup
       ├─ Services discover each other
       └─ Health checks managed
```

## Pattern Flow Diagram

```
First Call to getInstance()
    ↓
Check: Does instance exist?
    ├─ YES → Return existing instance
    └─ NO  → Create new instance
            → Initialize
            → Return

Subsequent Calls
    ↓
Return existing instance (always same object)
```

## Thread Safety Analysis

### C++11 Magic Statics

```cpp
static T& getInstance() {
    static T instance;  // ← Thread-safe initialization
    return instance;
}

// This is guaranteed by C++ standard:
// - First thread to call initializes
// - Other threads wait
// - All get same instance
```

### Before C++11 (Not Used Here)

```cpp
// ❌ Not thread-safe (race condition)
static T* instance = nullptr;
if (!instance) {
    instance = new T();  // Multiple threads could create instances
}

// ✅ Thread-safe (double-checked locking)
static std::mutex mtx;
if (!instance) {
    std::lock_guard<std::mutex> lock(mtx);
    if (!instance) {
        instance = new T();
    }
}
```

## Key Design Decisions

### ✅ Why Private Constructor?

```cpp
private:
    Singleton() = default;  // Private constructor
    
// Prevents accidental instantiation
// Singleton<Logger> logger;  // ❌ Compile error!

// Forces use of getInstance
Logger& logger = Singleton<Logger>::getInstance();  // ✅ Only way
```

### ✅ Why Delete Copy/Assignment?

```cpp
Logger(const Logger&) = delete;           // No copy constructor
Logger& operator=(const Logger&) = delete; // No assignment

// Prevents copying
Logger& logger2 = logger1;  // ❌ Compile error!

// Ensures single instance
Logger& logger2 = Singleton<Logger>::getInstance();  // ✅ Same instance
```

### ✅ Why Lazy Initialization?

```cpp
// Instance created on first use
Logger& logger = Logger::getInstance();  // Created here

// Advantages:
// - Expensive initialization deferred
// - May never be needed (doesn't create)
// - Dependencies available (created after them)
```

## Comparison with Alternatives

### vs. Global Variable
```cpp
// ❌ Global variable: Uncontrolled initialization order
Logger global_logger;

// ✅ Singleton: Controlled initialization
Logger& logger = Logger::getInstance();
```

### vs. Static Member Variable
```cpp
// ❌ Static: Same issues as global, less control
class Logger {
    static Logger instance;  // ❌ Created at program start
};

// ✅ Singleton: Lazy initialization
Logger& logger = Logger::getInstance();  // ✅ Created when needed
```

### vs. Dependency Injection
```cpp
// ❌ Singleton: Hard to test, hidden dependencies
Logger& logger = Logger::getInstance();
logger.log("message");

// ✅ Dependency Injection: Testable, explicit
void process(Logger& logger) {
    logger.log("message");
}

// Use together: Inject singleton in normal code, mock in tests
```

## Singleton Anti-Patterns

### ❌ Don't Use Singleton as Global Variable Replacement

```cpp
// Bad: Hiding dependencies
class UserService {
    void process() {
        Logger& logger = Logger::getInstance();
        Database& db = Database::getInstance();
        // Hard to test - can't replace these
    }
};

// Good: Inject dependencies
class UserService {
    UserService(Logger& l, Database& d) : logger(l), db(d) {}
    void process() {
        logger.log("...");
        db.query("...");
        // Easy to test - use mock logger and database
    }
private:
    Logger& logger;
    Database& db;
};
```

### ❌ Don't Overuse Singletons

```cpp
// Bad: Singleton for everything
Singleton<UserManager>
Singleton<ProductManager>
Singleton<OrderManager>
Singleton<PaymentManager>
// Too many, creates hard dependencies

// Good: Singletons only for shared resources
Singleton<Logger>         // Shared logging
Singleton<ConfigManager>  // Shared configuration
Singleton<ServiceRegistry> // Service discovery

// Others use dependency injection
void processOrder(OrderManager& mgr, Logger& logger) {
    mgr.process(logger);
}
```

### ❌ Don't Create Singletons for Stateful Objects

```cpp
// Bad: Singleton with mutable state
class SessionManager : public Singleton<SessionManager> {
public:
    User currentUser;  // ❌ Global mutable state
};

// Good: Singleton for stateless utilities
class Logger : public Singleton<Logger> {
public:
    void log(const std::string& msg);  // ✅ Stateless operation
};
```

## Testing with Singletons

### Challenge: Testing Code Using Singletons

```cpp
// Hard to test: dependencies hidden
class OrderProcessor {
    void processOrder(Order order) {
        Logger& logger = Logger::getInstance();
        Database& db = Database::getInstance();
        // Can't mock logger or db
    }
};
```

### Solution 1: Inject the Singleton

```cpp
// Testable: dependencies explicit
class OrderProcessor {
public:
    OrderProcessor(Logger& l, Database& d) : logger(l), db(d) {}
    
    void processOrder(Order order) {
        logger.log("Processing");
        db.save(order);
    }
private:
    Logger& logger;
    Database& db;
};

// Production: Use singletons
Logger& logger = Logger::getInstance();
Database& db = Database::getInstance();
OrderProcessor processor(logger, db);

// Testing: Use mocks
MockLogger mock_logger;
MockDatabase mock_db;
OrderProcessor processor(mock_logger, mock_db);
```

### Solution 2: Mock Singleton (Advanced)

```cpp
// Can replace singleton with mock
class Logger {
public:
    static Logger& getInstance() {
        if (test_instance) return *test_instance;
        static Logger instance;
        return instance;
    }
    
    static void setTestInstance(Logger* mock) {
        test_instance = mock;
    }
    
private:
    static Logger* test_instance;
};

// In tests
MockLogger mock;
Logger::setTestInstance(&mock);
// Code now uses mock
Logger::setTestInstance(nullptr);  // Restore normal
```

## Singleton in Local Cloud Phases

| Component | Purpose | Phase |
|-----------|---------|-------|
| Logger | Global logging | 1 ✅ |
| PluginFactory | Plugin registry | 1 ✅ |
| ServiceRegistry | Service discovery | 2 🔄 |
| ConfigManager | Shared configuration | 2 🔄 |
| MessageBroker | RPC communication | 3 📋 |
| ResourceScheduler | Resource quotas | 4 📋 |
| AuthManager | Service authentication | 6 📋 |

## Testing Singleton Pattern

### Run Test
```bash
./bin/test_singelton
# Tests:
# - Only one instance created
# - Same instance returned on multiple calls
# - Lazy initialization
# - No global state issues
```

### Example Test
```cpp
#include "singelton.hpp"

class TestSingleton {
public:
    static int count;
    TestSingleton() { count++; }
};

int TestSingleton::count = 0;

int main() {
    // First call
    TestSingleton& s1 = Singleton<TestSingleton>::getInstance();
    assert(TestSingleton::count == 1);  // Created once
    
    // Second call
    TestSingleton& s2 = Singleton<TestSingleton>::getInstance();
    assert(TestSingleton::count == 1);  // Not created again
    
    // Same instance
    assert(&s1 == &s2);  // ✅ Same object
    
    return 0;
}
```

## Common Patterns with Singleton

### Logger Singleton with Levels
```cpp
class Logger : public Singleton<Logger> {
public:
    void log(Level level, const std::string& msg);
    void setLevel(Level level) { current_level = level; }
private:
    Level current_level = INFO;
};

// Use everywhere
Logger& logger = Logger::getInstance();
logger.log(DEBUG, "Starting...");
```

### Factory Singleton
```cpp
template <typename T>
class Factory : public Singleton<Factory<T>> {
public:
    void registerType(const std::string& name, Creator creator);
    T* create(const std::string& name);
};

// Use everywhere
Factory<IPlugin>& factory = Factory<IPlugin>::getInstance();
```

### Service Registry Singleton (Phase 2)
```cpp
class ServiceRegistry : public Singleton<ServiceRegistry> {
public:
    void registerService(const std::string& name, IService* svc);
    IService* discoverService(const std::string& name);
};
```

## Summary

| Aspect | Singleton Pattern |
|--------|-------------------|
| **Use When** | Need single global instance (logger, factory, registry) |
| **Avoids** | Multiple instances, global variables, initialization order issues |
| **Key Benefit** | Controlled global state with lazy initialization |
| **Complexity** | Very Low |
| **Thread-Safe** | Yes (C++11 magic statics) |
| **Testing** | Can be challenging (use dependency injection) |

---

**Location**: `design_patterns/singleton/`  
**Status**: ✅ Fully implemented and tested  
**Used By**: Logger, PluginFactory, ServiceRegistry
