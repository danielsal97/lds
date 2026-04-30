# Plugin System (`/plugins`)

## Purpose

This directory contains **plugin implementations** - the modules that are dynamically loaded at runtime by the Local Cloud system. Plugins are self-contained units of functionality that can be added, removed, or updated without restarting the main application.

## Directory Structure

```
plugins/
├─ README.md                          # This file
├─ include/                           # Plugin interface headers
│  └─ plugin_interface.hpp            # Base plugin definitions
└─ src/                               # Plugin implementations
   └─ sample_plugin.cpp               # Example plugin implementation
```

## What is a Plugin?

A plugin is a **shared library (.so file)** that:
1. Implements the plugin interface
2. Provides self-initialization via static constructors
3. Registers itself with the PluginFactory
4. Can be loaded/unloaded dynamically without restart
5. Operates independently from other plugins

## Example Plugin: SamplePlugin

### How It Works
```cpp
// Static constructor runs on dlopen()
__attribute__((constructor)) void init_sample_plugin() {
    SamplePlugin* plugin = new SamplePlugin();
    // Plugin auto-registers with Factory
}

// Destructor runs on dlclose()
SamplePlugin::~SamplePlugin() {
    // Cleanup happens automatically (RAII)
}
```

### Plugin Lifecycle

```
1. Plugin file created in monitored directory
                    ▼
2. DirMonitor detects filesystem event
                    ▼
3. PNP receives notification
                    ▼
4. Loader calls dlopen() on plugin file
                    ▼
5. Plugin constructor runs (static initializer)
                    ▼
6. Plugin registers with Factory
                    ▼
7. Plugin is ready for use
```

## Building Plugins

### Compile as Shared Library
```bash
# From project root
make plugins

# Or manually
g++ -fPIC -shared -o bin/sample_plugin.so plugins/src/sample_plugin.cpp
```

### Plugin Output
```
$ ./bin/sample_plugin.so
🚀 [SamplePlugin] Constructor - PLUGIN LOADED
🏭 [Factory] Registered: SamplePlugin
📋 Registered Plugins:
   - SamplePlugin
```

## Loading Plugins

### Via Loader (Direct)
```cpp
#include "loader.hpp"

Loader plugin_loader("./bin/sample_plugin.so");
// Plugin is loaded and initialized
// Destructor automatically unloads
```

### Via PNP (Directory Monitoring)
```cpp
#include "pnp.hpp"

PNP plugin_manager;
plugin_manager.monitorDirectory("/tmp/pnp_plugins");
// PNP will load any .so files added to the directory
```

### Via DirMonitor (Low-level)
```bash
# Monitor directory for changes
./bin/test_pnp_main &

# Copy plugin to monitored directory
cp bin/sample_plugin.so /tmp/pnp_plugins/my_plugin.so

# Plugin is automatically loaded
```

## Plugin Interface

### Minimal Plugin Implementation

```cpp
#include <iostream>

class MyPlugin {
public:
    MyPlugin() {
        std::cout << "🚀 [MyPlugin] Loaded" << std::endl;
        register_with_factory();
    }
    
    ~MyPlugin() {
        std::cout << "💥 [MyPlugin] Unloaded" << std::endl;
    }
    
private:
    void register_with_factory() {
        // Plugin registers itself for discovery
    }
};

// Auto-initialization
__attribute__((constructor)) void init() {
    new MyPlugin();
}
```

## Plugin Isolation

Each plugin:
- Runs in same process (fast inter-plugin communication)
- Shares memory space (careful thread safety needed)
- Has independent symbol resolution
- Can be version-controlled separately

## Plugin Naming Conventions

```
sample_plugin.so          # Generic plugin
database_service.so       # Database plugin (Phase 2)
api_server.so            # API plugin (Phase 2)
cache_service.so         # Cache plugin (Phase 2)
```

## Future Plugin Development (Phase 2+)

### Service Plugins
```cpp
class DatabaseService : public IService {
public:
    std::string getName() override { return "database"; }
    std::string getVersion() override { return "1.0.0"; }
    void healthCheck() override { /* validate connection */ }
};
```

### Plugin Dependencies
```
Phase 2: Plugins can declare dependencies
Phase 3: Plugin A can call Plugin B via RPC
Phase 4: Resource quotas per plugin
Phase 5: Persistent storage for plugins
Phase 6: Service-to-service auth between plugins
```

## Debugging Plugins

### Check Plugin Symbols
```bash
nm -D bin/sample_plugin.so | grep -E "init|SamplePlugin"
```

### Test Loading
```bash
./bin/test_plugin_load    # Direct load test
./bin/test_pnp_main       # Monitored load test
```

### Common Issues

| Issue | Solution |
|-------|----------|
| Plugin not found | Check file path, verify .so extension |
| Symbol not found | Verify all dependencies compiled, check `nm -D` |
| Segfault on load | Check constructor code, verify thread safety |
| Plugin not initialized | Verify static constructor runs, check Factory |

## Testing Plugins

### Single Plugin Test
```bash
./bin/test_plugin_load
# Expected: Plugin loads and unloads cleanly
```

### Multiple Plugin Test
```bash
./bin/test_pnp_main &
sleep 1
cp bin/sample_plugin.so /tmp/pnp_plugins/p1.so
cp bin/sample_plugin.so /tmp/pnp_plugins/p2.so
cp bin/sample_plugin.so /tmp/pnp_plugins/p3.so
# Expected: All three plugins load separately
```

## Architecture Integration

```
┌──────────────────────────────────┐
│    Plugins (Phase 1 - Current)   │
│  - Dynamic loading (.so files)   │
│  - Auto-initialization           │
│  - Self-registration             │
└──────────────────────────────────┘
           ▲
           │ orchestrated by
           │
┌──────────────────────────────────┐
│    Plugin Management (LDS)       │
│  - Loader (dlopen/dlclose)      │
│  - DirMonitor (inotify events)  │
│  - PNP (plugin orchestrator)    │
└──────────────────────────────────┘

Phase 2+: Services (plugins become services with discovery)
Phase 3+: RPC (services communicate via message broker)
```

---

**Phase**: 1 (Dynamic Plugin Loading) ✅  
**Status**: Working, extensible for Phase 2+
