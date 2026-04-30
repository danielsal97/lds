# Utilities & Framework Components (`/utilities`)

## Purpose

This directory contains **core framework components and utility classes** that form the foundation of the Local Cloud system. These are reusable, battle-tested components used throughout the project.

## Directory Structure

```
utilities/
├─ README.md                          # This file
├─ logger/                            # Logging system
├─ threading/                         # Threading utilities
└─ thread_safe_data_structures/       # Concurrent data structures
```

## Core Components

### 1. **Logger** (Observability)

**Location**: `logger/`

**Purpose**: Centralized logging for the entire system

**Features**:
- Singleton pattern (one global logger)
- Multiple log levels (DEBUG, INFO, WARN, ERROR)
- Filtering by level
- Thread-safe logging

**Usage**:
```cpp
#include "logger.hpp"

Logger& logger = Logger::getInstance();
logger.log(LogLevel::INFO, "System started");
logger.log(LogLevel::ERROR, "Connection failed");
```

**Test**: `./bin/test_logger`

---

### 2. **ThreadPool** (Concurrent Execution)

**Location**: `threading/`

**Purpose**: Execute tasks concurrently using worker threads

**Features**:
- Configurable worker thread count
- Thread-safe task queue
- Future-based result handling
- RAII resource management

**Architecture**:
```
ThreadPool (N threads)
    ├─ Worker 1
    ├─ Worker 2
    ├─ ...
    └─ Worker N
         ↑
         │ tasks
         │
    ┌────────────────────┐
    │  Thread-Safe Queue │
    │  (Work Queue)      │
    └────────────────────┘
```

**Usage**:
```cpp
#include "thread_pool.hpp"

ThreadPool pool(4);  // 4 worker threads

// Enqueue a task
auto future = pool.enqueue([](){ 
    return doWork(); 
});

// Wait for result
auto result = future.get();
```

**Test**: `./bin/test_thread_pool`

---

### 3. **Thread-Safe Data Structures**

**Location**: `thread_safe_data_structures/`

**Purpose**: Concurrent data structures for multi-threaded environments

**Components**:

#### Work Queue (WPQ)
- Thread-safe FIFO queue
- Blocking pop with timeout
- Used by ThreadPool

**Usage**:
```cpp
#include "wpq.hpp"

WorkQueue<Task> queue;
queue.push(task);
Task t = queue.pop();  // Blocks until item available
```

**Test**: `./bin/test_wpq`

---

## Framework Integration

### How Components Work Together

```
Dispatcher (Observer Pattern)
    ↓ notifies
┌──────────────────────────┐
│  ThreadPool (threading)  │
├──────────────────────────┤
│  - Worker threads       │
│  - Work queue (WPQ)     │
│  - RAII management      │
└──────────────────────────┘
    ↓ logs to
Logger (Singleton)
    ↓ outputs to
Console/File
```

### Usage Across Project

| Component | Used By | Purpose |
|-----------|---------|---------|
| Logger | All components | Debugging & observability |
| ThreadPool | Future phases | Async task execution |
| Work Queue | ThreadPool | Task distribution |

## Logger Details

### Log Levels

```cpp
enum class LogLevel {
    DEBUG,   // Detailed diagnostic information
    INFO,    // Informational messages
    WARN,    // Warning messages
    ERROR    // Error messages
};
```

### Log Configuration

```cpp
Logger& logger = Logger::getInstance();

// Set minimum log level
logger.setLevel(LogLevel::INFO);

// Only INFO, WARN, ERROR will be logged
logger.log(LogLevel::DEBUG, "ignored");  // Not shown
logger.log(LogLevel::INFO, "shown");     // Shown
```

### Thread Safety

Logger is thread-safe - multiple threads can log simultaneously.

```cpp
// Thread 1
Logger::getInstance().log(LogLevel::INFO, "Thread 1");

// Thread 2
Logger::getInstance().log(LogLevel::INFO, "Thread 2");

// Both messages appear, no corruption
```

## ThreadPool Details

### Task Execution Model

```
ThreadPool.enqueue(task)
    ↓
Add to work queue
    ↓
Idle worker thread takes task
    ↓
Execute task
    ↓
Return result via Future
    ↓
Main thread calls future.get()
```

### Worker Thread Lifecycle

```
Worker Thread
    ↓
Wait for task in queue
    ↓
Task available?
├─ YES → Execute task, return result
└─ NO  → Wait (blocking)

Destructor of ThreadPool
    ↓
Signal workers to stop
    ↓
Wait for all tasks to finish
    ↓
Cleanup threads
```

### Example: Parallel Processing

```cpp
#include "thread_pool.hpp"

ThreadPool pool(4);

// Submit 10 tasks
std::vector<std::future<int>> results;
for (int i = 0; i < 10; i++) {
    results.push_back(pool.enqueue([i]() {
        return expensiveComputation(i);
    }));
}

// Collect results
for (auto& future : results) {
    int result = future.get();
    std::cout << "Result: " << result << std::endl;
}
```

## Logging Best Practices

### ✅ Good Logging

```cpp
// Clear, informative messages
logger.log(LogLevel::INFO, "Database connection established");
logger.log(LogLevel::ERROR, "Failed to load plugin: " + filename);

// Appropriate levels
logger.log(LogLevel::DEBUG, "Processing item 5 of 10");
logger.log(LogLevel::WARN, "Retry attempt 2 of 3");
logger.log(LogLevel::INFO, "Service started successfully");
```

### ❌ Poor Logging

```cpp
// Vague messages
logger.log(LogLevel::INFO, "Something happened");

// Wrong level
logger.log(LogLevel::DEBUG, "CRITICAL SYSTEM ERROR");  // Should be ERROR

// Too verbose
logger.log(LogLevel::DEBUG, "x=1 y=2 z=3 a=4...");  // Too much data
```

## Performance Considerations

### Logger
- **Overhead**: Minimal (synchronous write)
- **Use Case**: All levels of the system

### ThreadPool
- **Optimal Tasks**: CPU-bound work that takes >1ms
- **Avoid For**: Very quick operations (<100µs)
- **Overhead**: Thread creation is cheap (created once), task queue operations are O(1)

### Work Queue
- **Scalability**: O(1) push/pop operations
- **Memory**: Grows with queue depth
- **Thread Safety**: Lock-free operations where possible

## Configuration & Tuning

### ThreadPool Sizing

```cpp
// Number of worker threads
int optimal_threads = std::thread::hardware_concurrency();

ThreadPool pool(optimal_threads);

// Or specific count
ThreadPool pool(4);  // 4 workers for quad-core
```

### Logger Level Control

```cpp
// Development: verbose
Logger::getInstance().setLevel(LogLevel::DEBUG);

// Production: less verbose
Logger::getInstance().setLevel(LogLevel::INFO);
```

## Testing Utilities

All components are thoroughly tested:

```bash
# Logger tests
./bin/test_logger
# Tests: log output, filtering, thread safety

# ThreadPool tests
./bin/test_thread_pool
# Tests: task execution, multiple threads, cleanup

# Work queue tests
./bin/test_wpq
# Tests: FIFO order, blocking behavior, thread safety
```

## Integration Examples

### Example 1: Logging from ThreadPool Tasks

```cpp
#include "thread_pool.hpp"
#include "logger.hpp"

ThreadPool pool(2);

pool.enqueue([](){ 
    Logger::getInstance().log(LogLevel::INFO, "Task 1 executing");
    return doWork();
});

pool.enqueue([](){ 
    Logger::getInstance().log(LogLevel::INFO, "Task 2 executing");
    return doWork();
});
```

### Example 2: Multi-threaded Logging

```cpp
ThreadPool pool(4);

for (int i = 0; i < 10; i++) {
    pool.enqueue([i](){
        Logger& logger = Logger::getInstance();
        logger.log(LogLevel::INFO, 
                   "Processing item " + std::to_string(i));
        processItem(i);
    });
}
```

### Example 3: Async Processing with Dispatcher

```cpp
#include "Dispatcher.hpp"
#include "thread_pool.hpp"

ThreadPool pool(4);
Dispatcher<FileEvent> dispatcher;

CallBack<FileEvent, Processor> processor(
    &dispatcher, 
    &my_processor,
    &Processor::processEvent
);

// File events trigger async processing
void onFileCreated(const FileEvent& event) {
    pool.enqueue([event](){
        Logger::getInstance().log(LogLevel::INFO,
            "Processing file: " + event.filename);
        processFile(event.filename);
    });
}
```

## Architecture Layers

```
┌────────────────────────────────────┐
│    Application Layer               │
│    (uses utilities for features)   │
└────────────────────────────────────┘
            ↑
┌────────────────────────────────────┐
│    Utilities & Frameworks          │
│  ├─ Logger (Singleton)            │
│  ├─ ThreadPool (Concurrent)        │
│  ├─ Work Queue (Thread-safe)       │
│  └─ RAII Management               │
└────────────────────────────────────┘
            ↑
┌────────────────────────────────────┐
│    OS/System APIs                  │
│  ├─ pthreads                       │
│  ├─ Mutexes & Conditions           │
│  └─ File I/O                       │
└────────────────────────────────────┘
```

## Future Enhancements

### Phase 2+
- **Structured Logging**: JSON output for machine parsing
- **Log Rotation**: Manage log file sizes
- **Remote Logging**: Send logs to central server
- **Performance Metrics**: Track ThreadPool statistics

## Debugging Utilities

### Thread Debugging

```bash
# List all threads
gdb ./bin/test_thread_pool
(gdb) info threads
(gdb) thread apply all bt  # All backtraces

# Detect race conditions
valgrind --tool=helgrind ./bin/test_thread_pool
```

### Logger Debugging

```cpp
// Enable all logs temporarily
Logger::getInstance().setLevel(LogLevel::DEBUG);

// Add file location to logs
logger.log(LogLevel::DEBUG, __FILE__ + ":"  + 
          std::to_string(__LINE__) + ": Message");
```

## Summary

| Component | Purpose | Status | Tests |
|-----------|---------|--------|-------|
| Logger | Centralized logging | ✅ Complete | test_logger |
| ThreadPool | Concurrent execution | ✅ Complete | test_thread_pool |
| Work Queue | Thread-safe queue | ✅ Complete | test_wpq |

---

**Phase**: 1 (Dynamic Plugin Loading)  
**Status**: ✅ Core utilities ready  
**Used By**: All components, all future phases
