# Unit Tests (`/test/unit/`)

## Purpose

This directory contains **individual unit test source files** - tests for specific components of the Local Cloud system. These files are compiled into the test binaries in `/bin/`.

## Test Files

| Test File | Component | Validates | Binary |
|-----------|-----------|-----------|--------|
| `test_plugin_load.cpp` | Loader, Plugin System | Direct plugin loading | `test_plugin_load` |
| `test_pnp_main.cpp` | PNP, DirMonitor | Filesystem monitoring & loading | `test_pnp_main` |
| `test_dir_monitor.cpp` | DirMonitor | File event detection | `test_dir_monitor` |
| `test_inotify_debug.cpp` | inotify | Low-level inotify API | `test_inotify_debug` |
| `test_command_demo.cpp` | Command Pattern | Command creation & execution | `test_command_demo` |
| `test_logger.cpp` | Logger | Log output & levels | `test_logger` |
| `test_msg_broker.cpp` | MessageBroker | Message passing | `test_msg_broker` |
| `test_singelton.cpp` | Singleton | Instance uniqueness | `test_singelton` |
| `test_thread_pool.cpp` | ThreadPool | Task execution | `test_thread_pool` |
| `test_wpq.cpp` | Work Queue | Thread-safe queueing | `test_wpq` |

## Running Tests

### Run All Tests
```bash
# From project root
make test

# Or run individual tests
./bin/test_plugin_load
./bin/test_thread_pool
./bin/test_logger
```

### Run Specific Test
```bash
./bin/test_plugin_load
# Expected: Plugin loads and unloads cleanly

./bin/test_pnp_main
# Expected: Multiple plugins load from directory
```

## Test Structure

### Typical Test File

```cpp
// test/unit/test_component.cpp

#include <iostream>
#include "framework_header.hpp"

int main() {
    // Setup
    Component component;
    
    // Test case 1
    bool result1 = component.operation();
    assert(result1 == expected);
    std::cout << "✅ Test 1 passed" << std::endl;
    
    // Test case 2
    bool result2 = component.anotherOp();
    assert(result2 == expected);
    std::cout << "✅ Test 2 passed" << std::endl;
    
    // Cleanup
    // RAII handles automatic cleanup
    
    std::cout << "✅ All tests passed" << std::endl;
    return 0;
}
```

## Test Categories

### Phase 1: Plugin Loading Tests

These tests validate the dynamic plugin loading system:

- **test_plugin_load.cpp** - Single plugin loading
  - Load a .so file
  - Verify static constructor runs
  - Verify destructor runs on unload
  
- **test_pnp_main.cpp** - Multiple plugins with monitoring
  - Monitor directory for changes
  - Load plugins when files appear
  - Verify concurrent loading
  
- **test_dir_monitor.cpp** - Filesystem event detection
  - Create/modify/delete files
  - Verify inotify detects events
  - Verify event ordering

- **test_inotify_debug.cpp** - Low-level inotify API
  - Direct inotify testing
  - Verify Linux kernel API behavior
  - Debug inotify issues

### Framework Tests

These tests validate core framework components:

- **test_logger.cpp** - Logging system
  - Log at various levels
  - Verify filtering
  - Thread safety
  
- **test_thread_pool.cpp** - Concurrent execution
  - Enqueue tasks
  - Execute in parallel
  - Collect results
  
- **test_wpq.cpp** - Work queue
  - FIFO ordering
  - Blocking operations
  - Thread safety
  
- **test_singelton.cpp** - Singleton pattern
  - One instance created
  - Global access
  - Thread safety
  
- **test_msg_broker.cpp** - Message passing
  - Send/receive messages
  - Request/response pattern
  - Multiple clients

- **test_command_demo.cpp** - Command pattern
  - Create commands
  - Execute with parameters
  - Return results

## Adding New Tests

### Step 1: Create Test File

```cpp
// test/unit/test_new_component.cpp
#include <iostream>
#include "new_component.hpp"

int main() {
    std::cout << "Testing NewComponent..." << std::endl;
    
    NewComponent comp;
    assert(comp.validate());
    
    std::cout << "✅ All tests passed" << std::endl;
    return 0;
}
```

### Step 2: Update Makefile

```makefile
# In Makefile
test_new_component: test/unit/test_new_component.cpp
	$(CXX) $(CFLAGS) -o bin/test_new_component \
	       test/unit/test_new_component.cpp $(LIBS)
```

### Step 3: Run Test

```bash
make test_new_component
./bin/test_new_component
```

## Expected Test Output

### Successful Test
```
Testing Component...
✅ Validation passed
✅ Operation 1 succeeded
✅ Operation 2 succeeded
✅ All tests passed
```

### Failed Test
```
Testing Component...
✅ Validation passed
❌ Operation 1 failed
Assertion failed: expected == actual
```

## Testing Best Practices

### ✅ Good Tests

```cpp
// Clear test purpose
std::cout << "Testing plugin initialization..." << std::endl;

// Specific assertions
assert(plugin.isLoaded() == true);
assert(plugin.getName() == "TestPlugin");

// Cleanup resources
delete plugin;

// Clear results
std::cout << "✅ Plugin initialization test passed" << std::endl;
```

### ❌ Poor Tests

```cpp
// Vague purpose
std::cout << "Running tests..." << std::endl;

// Non-specific assertions
assert(result);  // What does result represent?

// Resource leaks
MyObject* obj = new MyObject();
// No delete - memory leak in test

// Hidden failures
if (plugin == nullptr) {
    // Silently continues, should fail test
}
```

## Debugging Tests

### Using GDB

```bash
# Run test under debugger
gdb ./bin/test_plugin_load

(gdb) run
(gdb) bt          # Show backtrace if crashes
(gdb) print var   # Print variable value
(gdb) quit
```

### Memory Checking

```bash
# Check for memory leaks
valgrind ./bin/test_plugin_load

# Profile memory usage
valgrind --tool=massif ./bin/test_plugin_load
```

### Race Condition Detection

```bash
# Check for thread races
valgrind --tool=helgrind ./bin/test_thread_pool
```

## Test Coverage

### Phase 1 Validation Checklist

- [x] Plugin loads successfully
- [x] Plugin unloads cleanly
- [x] Multiple plugins load simultaneously
- [x] Filesystem events detected
- [x] Auto-initialization works
- [x] Error handling works
- [x] No memory leaks
- [x] Thread-safe operations

### Component Testing

- [x] Logger outputs messages
- [x] ThreadPool executes tasks
- [x] Work queue is FIFO
- [x] Singleton creates one instance
- [x] MessageBroker routes messages
- [x] Commands execute with parameters

## Integration with Main Tests

```
/test              (testing framework)
    ├─ unit/      (component tests)
    ├─ test_*.cpp (test scripts)
    └─ BEHAVIOR_GUIDE.md

/bin               (compiled binaries)
    ├─ test_*     (test executables)
    └─ LDS        (main application)
```

## Test Execution Flow

```
make test
    ├─ Compile all test files
    ├─ Link with framework components
    └─ Run all test binaries
        ├─ test_plugin_load
        ├─ test_pnp_main
        ├─ test_thread_pool
        └─ ... (others)
```

## Continuous Integration

For CI/CD pipelines:

```bash
#!/bin/bash
# .github/workflows/test.yml or similar

# Build
make clean
make

# Run tests
make test

# Check exit code
if [ $? -eq 0 ]; then
    echo "✅ All tests passed"
else
    echo "❌ Some tests failed"
    exit 1
fi
```

## Future Test Coverage

### Phase 2+ Tests

| Phase | Component | Test |
|-------|-----------|------|
| 2 | ServiceRegistry | test_service_registry.cpp |
| 2 | ServiceDiscovery | test_service_discovery.cpp |
| 3 | MessageBroker | test_msg_broker.cpp (existing) |
| 3 | RPC | test_rpc_invocation.cpp |
| 4 | Scheduler | test_scheduler.cpp |
| 5 | Storage | test_storage.cpp |
| 6 | Security | test_security.cpp |

## Test Files Reference

### Quick Lookup

```bash
# Find test for component
grep -r "className" test/unit/*.cpp

# Find what tests exist
ls -1 test/unit/test_*.cpp | sed 's|test/unit/||' | sed 's|\.cpp||'

# Run specific test
./bin/test_component_name
```

---

**Location**: `test/unit/`  
**Status**: ✅ Active test suite  
**Maintenance**: Add tests for new components
