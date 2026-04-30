# Testing (`/test`)

## Purpose

This directory contains **test files, test utilities, and testing documentation** for the Local Cloud project. It includes unit tests, integration tests, and testing guides for validating the entire system.

## Directory Structure

```
test/
├─ README.md                          # This file
├─ unit/                              # Unit tests for individual components
├─ BEHAVIOR_GUIDE.md                  # Test behavior documentation
├─ TEST_MSG_BROKER_GUIDE.md          # Message broker testing guide
├─ test_readwrite.sh                  # Shell script for read/write testing
└─ test_signals.sh                    # Shell script for signal testing
```

## Contents

### Test Guides
| File | Purpose |
|------|---------|
| `BEHAVIOR_GUIDE.md` | Comprehensive guide to expected system behaviors and test scenarios |
| `TEST_MSG_BROKER_GUIDE.md` | Specific guide for testing the message broker component |

### Test Scripts
| Script | Purpose |
|--------|---------|
| `test_readwrite.sh` | Tests file reading/writing operations |
| `test_signals.sh` | Tests signal handling in the system |

### Unit Tests
| Directory | Purpose |
|-----------|---------|
| `unit/` | Individual component unit tests |

## Running Tests

### Quick Test Suite
```bash
# Run all tests
make test

# Run specific test binary
./bin/test_plugin_load
./bin/test_thread_pool
./bin/test_logger
```

### Message Broker Testing
```bash
# See detailed test procedures
cat test/TEST_MSG_BROKER_GUIDE.md

# Run message broker test
./bin/test_msg_broker
```

### System Behavior Testing
```bash
# See all expected behaviors
cat test/BEHAVIOR_GUIDE.md

# Run read/write tests
bash test/test_readwrite.sh

# Run signal tests
bash test/test_signals.sh
```

## Test Coverage

### Phase 1: Dynamic Plugin Loading ✅
- [x] Plugin loading (dlopen)
- [x] Plugin unloading (dlclose)
- [x] Filesystem monitoring (inotify)
- [x] Multiple plugin simultaneous loading
- [x] Plugin auto-initialization

### Framework Components ✅
- [x] Dispatcher event notification
- [x] CallBack observer pattern
- [x] ThreadPool concurrent execution
- [x] Logger message output
- [x] Singleton pattern
- [x] Factory pattern
- [x] Command pattern
- [x] Message broker

## Test Execution Flow

```
make test
│
├─ test_plugin_load
│  └─ Validates: Single plugin loading/unloading
│
├─ test_pnp_main
│  └─ Validates: Multiple plugins, filesystem events
│
├─ test_thread_pool
│  └─ Validates: Concurrent task execution
│
├─ test_msg_broker
│  └─ Validates: Message passing between components
│
├─ test_logger
│  └─ Validates: Log output and filtering
│
└─ ... other framework tests
```

## Expected Test Output

### Successful Plugin Load
```
🚀 [SamplePlugin] Constructor - PLUGIN LOADED
💥 [SamplePlugin] Destructor - PLUGIN UNLOADED
✅ Test passed
```

### Filesystem Monitoring
```
🔔 [DirMonitor] Detected: CREATE: plugin1.so
📨 [PNP] Event received: CREATE: plugin1.so
✅ Loaded successfully
```

## Troubleshooting Tests

### Plugin Load Fails
1. Check `/tmp/pnp_plugins` directory exists
2. Verify plugin file is executable
3. Check for symbol resolution errors: `nm -D plugin.so`

### Thread Pool Tests Hang
1. Check for deadlock in shared resources
2. Verify thread count matches system capacity
3. Check system memory availability

### Message Broker Tests Fail
1. Verify thread safety of queue implementation
2. Check for message ordering issues
3. Validate serialization/deserialization

## Adding New Tests

1. Create test file in appropriate location
2. Follow naming convention: `test_<component>.cpp`
3. Use existing framework components
4. Add to Makefile targets
5. Document expected behavior

### Test Template
```cpp
#include <iostream>
#include "framework_header.hpp"

int main() {
    // Setup
    MyComponent component;
    
    // Test
    bool result = component.operation();
    
    // Verify
    assert(result == expected);
    
    // Cleanup
    // RAII handles this automatically
    
    std::cout << "✅ Test passed" << std::endl;
    return 0;
}
```

## Next Phase

Phase 2 will add:
- Service registry tests
- Service discovery tests
- Health check validation
- Integration tests with multiple services

---

**Phase**: 1 (Dynamic Plugin Loading)  
**Status**: ✅ Test suite complete and passing
