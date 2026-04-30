# Factory Pattern (`/design_patterns/factory/`)

## Pattern Overview

**Purpose**: Provide an interface for creating objects without specifying their exact classes.

**Problem It Solves**:
- How to create objects without hardcoding types?
- How to decouple object creation from usage?
- How to enable runtime object creation and registration?
- How to allow plugins to self-register?

**Real-World Analogy**: Like a pizza factory - you ask for "Margherita" and the factory creates it. You don't need to know how it's made.

## Why Use Factory Pattern?

✅ **Decoupling**: Clients don't know concrete classes
✅ **Extensibility**: Add new types without changing client code
✅ **Centralized Creation**: All object creation logic in one place
✅ **Plugin Support**: Self-registering implementations
✅ **Polymorphism**: Create objects by type name
✅ **Runtime Registration**: Add types dynamically (plugins)

## Implementation Structure

```
design_patterns/factory/
├─ include/              # Headers
│  ├─ factory.hpp        # Factory interface
│  └─ (other pattern headers)
└─ src/                  # Implementation
   └─ (implementation in headers - templates)
```

## Core Components

### 1. Factory (Object Creator)

```cpp
template <typename T>
class Factory {
public:
    static Factory& getInstance();
    
    void registerType(const std::string& name, 
                     std::function<T*()> creator);
    T* create(const std::string& name);
    std::vector<std::string> listTypes();
};
```

**Responsibility**: Register creators and create objects by type name

**Key Features**:
- Singleton instance (global access)
- Type registration with creator functions
- Runtime object creation
- Type listing for discovery

---

### 2. Creator Function

```cpp
// Function that knows how to create an object
std::function<Plugin*()> creator = []() {
    return new ConcretePlugin();
};

// Register with factory
factory.registerType("ConcretePlugin", creator);

// Create by name
Plugin* plugin = factory.create("ConcretePlugin");
```

---

### 3. Plugin Self-Registration

```cpp
class MyPlugin {
public:
    MyPlugin() {
        // Register myself with the factory
        PluginFactory::getInstance()
            .registerType("MyPlugin", []{ return new MyPlugin(); });
    }
};

// Auto-registration on plugin load
__attribute__((constructor)) void init_plugin() {
    new MyPlugin();  // Constructor registers it
}
```

## Usage Example

### Scenario: Plugin Loading System

```cpp
#include "factory.hpp"
#include <iostream>

// Base interface for all plugins
class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual void execute() = 0;
};

// Concrete implementation 1
class DatabasePlugin : public IPlugin {
public:
    void execute() override {
        std::cout << "📊 Database plugin running" << std::endl;
    }
};

// Concrete implementation 2
class CachePlugin : public IPlugin {
public:
    void execute() override {
        std::cout << "⚡ Cache plugin running" << std::endl;
    }
};

// Registration (happens once at startup)
void registerPlugins() {
    auto& factory = PluginFactory<IPlugin>::getInstance();
    
    factory.registerType("database", 
        []() { return new DatabasePlugin(); });
    
    factory.registerType("cache",
        []() { return new CachePlugin(); });
}

// Usage: Create plugins by name
int main() {
    registerPlugins();
    
    auto& factory = PluginFactory<IPlugin>::getInstance();
    
    // List available plugins
    auto plugins = factory.listTypes();
    for (const auto& name : plugins) {
        std::cout << "✅ Available: " << name << std::endl;
    }
    
    // Create by name
    IPlugin* db = factory.create("database");
    db->execute();  // Output: 📊 Database plugin running
    
    IPlugin* cache = factory.create("cache");
    cache->execute();  // Output: ⚡ Cache plugin running
    
    delete db;
    delete cache;
    
    return 0;
}
```

## Used In Local Cloud Project

### Phase 1: Dynamic Plugin Loading

**Loader → Plugins → PluginFactory**

```
┌──────────────────┐
│  Loader          │  Loads plugin.so
└────────┬─────────┘
         │ dlopen()
         ↓
┌──────────────────────────────────┐
│  Plugin Constructor               │
│  (runs as static initializer)    │
└────────┬─────────────────────────┘
         │ registerType(...)
         ↓
┌──────────────────┐
│  PluginFactory   │  Registry of all plugins
└──────────────────┘
```

### Code in Project

```cpp
// In plugin source file
class SamplePlugin {
public:
    SamplePlugin() {
        std::cout << "🚀 [SamplePlugin] Loaded" << std::endl;
        
        // Register with factory
        PluginFactory::getInstance()
            .registerType("SamplePlugin", 
                         []() { return new SamplePlugin(); });
    }
};

// Auto-initialization on plugin load
__attribute__((constructor)) void init_sample_plugin() {
    new SamplePlugin();
}
```

## Pattern Flow Diagram

```
1. Registration (happens once)
   Constructor → registerType("Name", creator_function)
               → Factory stores entry

2. Creation (runtime)
   create("Name") → Lookup entry in factory
                 → Call creator_function
                 → Return new object

3. Discovery (query)
   listTypes() → Return all registered type names
```

## Key Design Decisions

### ✅ Why Use Singleton Factory?

```cpp
// Single global registry
PluginFactory& factory = PluginFactory::getInstance();

// All code uses same factory
// No duplicate registrations
// One source of truth
```

### ✅ Why Use Function Objects as Creators?

```cpp
// Flexible: lambda, function pointer, std::function
factory.registerType("A", []() { return new ClassA(); });
factory.registerType("B", createClassB);  // function pointer
factory.registerType("C", std::bind(&Creator::create, &obj));

// Can capture context
factory.registerType("D", [config]() { 
    return new ClassD(config); 
});
```

### ✅ Why Not Use RTTI (Runtime Type Info)?

```cpp
// ❌ Old approach: RTTI + reflection
// Requires static class registry
// Complex and error-prone

// ✅ Our approach: Explicit registration
// Simple, safe, and flexible
// Works with plugins and dynamic loading
```

## Comparison with Alternatives

### vs. Direct Constructor Calls
```cpp
// ❌ Direct: Type must be known at compile time
MyPlugin* plugin = new MyPlugin();

// ✅ Factory: Type chosen at runtime
std::string plugin_name = config.get("plugin");
MyPlugin* plugin = factory.create(plugin_name);
```

### vs. Abstract Base Class Only
```cpp
// ❌ Incomplete: Base class exists but no way to create
class IPlugin {
    virtual void execute() = 0;
};

// ✅ Factory: Concrete way to instantiate
Factory<IPlugin> factory;
IPlugin* p = factory.create("DatabasePlugin");
```

### vs. Hardcoded Type Mapping
```cpp
// ❌ Hardcoded: Must recompile to add new type
if (name == "A") return new ClassA();
if (name == "B") return new ClassB();
// Adding ClassC requires code change

// ✅ Factory: Add new types at runtime
factory.registerType("C", []() { return new ClassC(); });
```

## Plugin Self-Registration Pattern

### How Plugins Register Themselves

```
Plugin Loading Process
    ↓
dlopen(plugin.so)
    ↓
Static initializer runs
    ↓
Plugin constructor executes
    ↓
Constructor calls:
    PluginFactory::getInstance()
        .registerType("PluginName", creator_function)
    ↓
Plugin is now discoverable
```

### Why This Works

1. **Automatic**: No explicit registration code needed
2. **Modular**: Plugin handles its own registration
3. **Scalable**: New plugins don't need factory changes
4. **Decoupled**: Factory doesn't know about plugin types

## Type Registration Scenarios

### Scenario 1: Simple Plugin
```cpp
class SimplePlugin {
public:
    SimplePlugin() {
        PluginFactory::getInstance()
            .registerType("Simple", []() { 
                return new SimplePlugin(); 
            });
    }
};

__attribute__((constructor)) void init() {
    new SimplePlugin();
}
```

### Scenario 2: Plugin with Configuration
```cpp
class ConfigurablePlugin {
public:
    ConfigurablePlugin(const Config& cfg) : config(cfg) {}
    
    void registerFactory() {
        PluginFactory::getInstance()
            .registerType("Configurable", 
                [this]() { return new ConfigurablePlugin(config); });
    }
private:
    Config config;
};

__attribute__((constructor)) void init() {
    Config cfg = loadConfig();
    ConfigurablePlugin* plugin = new ConfigurablePlugin(cfg);
    plugin->registerFactory();
}
```

### Scenario 3: Plugin Factory (Phase 2+)
```cpp
class ServiceFactory : public IService {
public:
    ServiceFactory() {
        ServiceRegistry::getInstance()
            .registerType("ServiceFactory", 
                []() { return new ServiceFactory(); });
    }
};
```

## Type Safety

### Compile-Time Type Safety

```cpp
// Template ensures type consistency
Factory<IPlugin> plugin_factory;
Factory<ICommand> command_factory;

// Can't accidentally mix types
plugin_factory.registerType("P", []() { 
    return new MyPlugin();  // ✅ IPlugin
});

// command_factory.registerType("C", []() { 
//     return new MyPlugin();  // ❌ Compile error! Wrong type
// });
```

### Runtime Type Discovery

```cpp
// Can query available types
auto types = factory.listTypes();
for (const auto& type : types) {
    std::cout << type << std::endl;
}

// Can create if available
if (factory.exists("DatabasePlugin")) {
    IPlugin* p = factory.create("DatabasePlugin");
}
```

## Error Handling

```cpp
// Scenario: Plugin not found
try {
    IPlugin* plugin = factory.create("NonExistent");
    // What to do?
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}

// Better: Check before creating
if (factory.exists("DatabasePlugin")) {
    IPlugin* plugin = factory.create("DatabasePlugin");
} else {
    std::cerr << "Plugin not found" << std::endl;
}
```

## Testing Factory Pattern

### Run Test
```bash
./bin/test_plugin_load
# Tests:
# - Plugin registration via factory
# - Object creation by type name
# - Multiple plugins
# - Cleanup on plugin unload
```

### Example Test
```cpp
#include "factory.hpp"

class TestPlugin {
public:
    int id;
    TestPlugin(int i) : id(i) {}
};

int main() {
    auto& factory = Factory<TestPlugin>::getInstance();
    
    // Register
    int counter = 0;
    factory.registerType("Test1", [&]() { 
        return new TestPlugin(counter++); 
    });
    
    // Create
    TestPlugin* p1 = factory.create("Test1");  // id = 0
    TestPlugin* p2 = factory.create("Test1");  // id = 1
    
    assert(p1->id == 0);
    assert(p2->id == 1);
    
    // List types
    auto types = factory.listTypes();
    assert(types.size() == 1);
    assert(types[0] == "Test1");
    
    delete p1;
    delete p2;
    
    return 0;
}
```

## Integration with Other Patterns

### Factory + Singleton
```cpp
// Singleton factory (global registry)
Factory<T>& factory = Factory<T>::getInstance();
```

### Factory + Observer
```cpp
// Dispatcher<T> uses factory to create handlers
Dispatcher<Event> disp;
Handler* handler = factory.create("MyHandler");
```

### Factory + Command
```cpp
// Create command objects by name
ICommand* cmd = command_factory.create("ExecuteQuery");
Result result = cmd->execute();
```

## Extension Points

### Plugin Versioning
```cpp
factory.registerType("DatabasePlugin:1.0", 
    []() { return new DatabasePluginV1(); });
factory.registerType("DatabasePlugin:2.0", 
    []() { return new DatabasePluginV2(); });
```

### Lazy Initialization
```cpp
factory.registerType("ExpensivePlugin", []() {
    if (!initialized) {
        initialize();
        initialized = true;
    }
    return new ExpensivePlugin();
});
```

### Plugin Metadata
```cpp
struct PluginMetadata {
    std::string name;
    std::string version;
    std::string author;
    std::vector<std::string> dependencies;
};

factory.registerMetadata("DatabasePlugin", metadata);
auto meta = factory.getMetadata("DatabasePlugin");
```

## Summary

| Aspect | Factory Pattern |
|--------|-----------------|
| **Use When** | Need dynamic object creation by type name |
| **Avoids** | Hardcoded types, tight coupling to concrete classes |
| **Key Benefit** | Extensible without modifying factory code |
| **Complexity** | Low to Medium |
| **Plugin Integration** | Excellent for self-registering plugins |

---

**Location**: `design_patterns/factory/`  
**Status**: ✅ Fully implemented and tested  
**Used By**: Plugin system, all extensible components
