# Command Pattern (`/design_patterns/command/`)

## Pattern Overview

**Purpose**: Encapsulate a request as an object, allowing parameterization of clients with different requests, queueing, and logging.

**Problem It Solves**:
- How to decouple command invokers from command executors?
- How to queue, schedule, or log commands?
- How to support undo/redo operations?
- How to parameterize methods with different arguments?

**Real-World Analogy**: Like a restaurant - a waiter (invoker) takes orders (commands) from customers and gives them to the chef (executor). The order is a request object that encapsulates what needs to be done.

## Why Use Command Pattern?

✅ **Decoupling**: Invoker doesn't need to know executor details
✅ **Queueing**: Commands can be queued for later execution
✅ **Undo/Redo**: Easy to implement with command history
✅ **Logging**: Record and replay commands
✅ **Scheduling**: Execute commands at specific times
✅ **Extensibility**: Add new commands without changing invoker

## Implementation Structure

```
design_patterns/command/
├─ include/              # Headers
│  └─ ICommand.hpp       # Command interface
└─ src/                  # Implementation
   └─ ICommand.cpp       # Command implementation
```

## Core Component

### ICommand Interface

```cpp
template <typename Args, typename Return>
class ICommand {
public:
    virtual ~ICommand() = default;
    virtual Return execute(const Args& args) = 0;
    virtual std::string getName() const { return "Command"; }
};
```

**Responsibility**: Define the contract for all commands

**Key Features**:
- Two template parameters: Args (input) and Return (output)
- Pure virtual execute method
- Type-safe parameter passing
- Optional name for logging/debugging

---

## Usage Example

### Scenario: Database Query Execution

```cpp
#include "ICommand.hpp"
#include <iostream>
#include <string>

// Define argument and return types
struct QueryArgs {
    std::string sql;
    int timeout;
};

struct QueryResult {
    bool success;
    std::string data;
};

// Concrete command
class ExecuteQueryCommand : public ICommand<QueryArgs, QueryResult> {
public:
    QueryResult execute(const QueryArgs& args) override {
        std::cout << "Executing: " << args.sql << std::endl;
        std::cout << "Timeout: " << args.timeout << "s" << std::endl;
        
        return QueryResult{true, "Query results..."};
    }
    
    std::string getName() const override {
        return "ExecuteQueryCommand";
    }
};

// Usage
int main() {
    // Create command with parameters
    ExecuteQueryCommand* cmd = new ExecuteQueryCommand();
    
    // Prepare arguments
    QueryArgs args{"SELECT * FROM users", 30};
    
    // Execute command
    QueryResult result = cmd->execute(args);
    
    std::cout << "Success: " << (result.success ? "yes" : "no") << std::endl;
    std::cout << "Data: " << result.data << std::endl;
    
    delete cmd;
    return 0;
}

// Output:
// Executing: SELECT * FROM users
// Timeout: 30s
// Success: yes
// Data: Query results...
```

## Used In Local Cloud Project

### Phase 1: Task Execution

**ThreadPool → Commands**

```
┌──────────────────┐
│  ThreadPool      │  Manages worker threads
└────────┬─────────┘
         │ enqueue(command)
         ↓
┌──────────────────────────────────┐
│  Work Queue                      │
│  (ICommand objects)              │
└────────┬─────────────────────────┘
         │
    ┌────┴────┐
    ↓         ↓
┌──────────┐ ┌──────────┐
│Command 1 │ │Command 2 │
│execute() │ │execute() │
└──────────┘ └──────────┘
```

### Phase 3: RPC Invocation (Planned)

```
RPC Request
    ↓
Deserialize to Command object
    ↓
MessageBroker::execute(command)
    ↓
Command::execute(args)
    ↓
Return result
```

### Code in Project

```cpp
// ThreadPool executes commands
threadpool.enqueue([]() {
    std::cout << "Task executed" << std::endl;
});

// Phase 3: RPC commands
auto cmd = rpc_factory.create("QueryDatabase", sql, timeout);
Result result = cmd->execute();
```

## Pattern Flow Diagram

```
1. Command Creation
   new ExecuteQueryCommand()

2. Parameter Setting
   query_args{sql, timeout}

3. Command Execution
   cmd->execute(args) → do work → return result

4. Result Handling
   if (result.success) { ... }
```

## Key Design Decisions

### ✅ Why Template Parameters?

```cpp
// Type-safe: Compiler verifies argument and return types
ICommand<QueryArgs, QueryResult> query_cmd;

// Query expects QueryArgs
QueryResult r = query_cmd->execute(QueryArgs{...});

// Can't pass wrong types
// QueryResult r = query_cmd->execute(5);  // ❌ Type error!
```

### ✅ Why Separate Args and Return?

```cpp
// Clean separation of input and output
ICommand<Args, Return>

// Args: What the command needs
// Return: What the command produces

// For void operations
ICommand<Args, void>  // No return value

// For no-arg commands
ICommand<void, Result>  // No input needed
```

### ✅ Why Package Request as Object?

```cpp
// ❌ Direct invocation: Tight coupling
executor.executeQuery(sql, timeout, connection);
// Must know all parameters, types, order

// ✅ Command object: Loose coupling
class ExecuteQueryCommand : public ICommand<QueryArgs, QueryResult> {
    QueryResult execute(const QueryArgs& args) override;
};

// Invoker doesn't need to know details
```

## Comparison with Alternatives

### vs. Direct Function Call
```cpp
// ❌ Direct: Type-unsafe, parameters unknown
result = executeQuery(sql, timeout);

// ✅ Command: Type-safe, self-documenting
ICommand<QueryArgs, QueryResult>* cmd = new QueryCommand();
result = cmd->execute({sql, timeout});
```

### vs. Function Pointers
```cpp
// ❌ Function pointer: No type safety, unsafe casting
void (*func_ptr)(void*) = executeQuery;
func_ptr((void*)args);  // Unsafe!

// ✅ Command: Type-safe, OOP
ICommand<QueryArgs, QueryResult>* cmd = new QueryCommand();
result = cmd->execute(args);  // Type-checked
```

### vs. Callback Functions
```cpp
// ❌ Callback: Hard to parameterize
callback(query_string);
callback(timeout);
callback(connection);

// ✅ Command: All parameters in one object
QueryArgs args{query_string, timeout, connection};
result = cmd->execute(args);
```

## Advanced Scenarios

### Command with Context

```cpp
struct DatabaseContext {
    std::string connection_string;
    int pool_size;
    bool logging_enabled;
};

class QueryCommand : public ICommand<QueryArgs, QueryResult> {
public:
    QueryCommand(const DatabaseContext& ctx) : context(ctx) {}
    
    QueryResult execute(const QueryArgs& args) override {
        // Use context in execution
        if (context.logging_enabled) {
            log("Executing query: " + args.sql);
        }
        return executeQuery(args);
    }
    
private:
    DatabaseContext context;
};
```

### Composite Commands

```cpp
class CompositeCommand : public ICommand<CompositeArgs, CompositeResult> {
public:
    void add(ICommand<Args, Result>* cmd) {
        commands.push_back(cmd);
    }
    
    CompositeResult execute(const CompositeArgs& args) override {
        for (auto cmd : commands) {
            cmd->execute(...);
        }
        return CompositeResult{...};
    }
    
private:
    std::vector<ICommand<Args, Result>*> commands;
};
```

### Macro Commands (Multiple Operations)

```cpp
class TransactionCommand : public ICommand<TxnArgs, TxnResult> {
    // BEGIN TRANSACTION
    // INSERT ...
    // UPDATE ...
    // DELETE ...
    // COMMIT
    TxnResult execute(const TxnArgs& args) override {
        db.begin();
        try {
            db.insert(...);
            db.update(...);
            db.delete(...);
            db.commit();
            return TxnResult{true, "Transaction committed"};
        } catch (...) {
            db.rollback();
            return TxnResult{false, "Transaction rolled back"};
        }
    }
};
```

## Undo/Redo Implementation

### Simple Undo Stack

```cpp
class UndoableCommand : public ICommand<Args, Result> {
public:
    virtual Result execute(const Args& args) = 0;
    virtual Result undo(const Args& args) = 0;
};

class CommandHistory {
public:
    Result execute(UndoableCommand* cmd, const Args& args) {
        Result result = cmd->execute(args);
        history.push_back(cmd);
        return result;
    }
    
    Result undo() {
        if (history.empty()) return Result{false};
        
        UndoableCommand* cmd = history.back();
        history.pop_back();
        return cmd->undo(...);
    }
    
private:
    std::vector<UndoableCommand*> history;
};
```

## Command Queuing

```cpp
class CommandQueue {
public:
    void enqueue(ICommand<Args, Result>* cmd, const Args& args) {
        queue.push({cmd, args});
    }
    
    Result execute_next() {
        if (queue.empty()) return Result{false};
        
        auto [cmd, args] = queue.front();
        queue.pop();
        return cmd->execute(args);
    }
    
    void execute_all() {
        while (!queue.empty()) {
            execute_next();
        }
    }
    
private:
    std::queue<std::pair<ICommand<Args, Result>*, Args>> queue;
};
```

## Error Handling in Commands

```cpp
template <typename Args, typename Return>
class SafeCommand : public ICommand<Args, Return> {
public:
    Return execute(const Args& args) override {
        try {
            return doExecute(args);
        } catch (const std::exception& e) {
            std::cerr << "Command error: " << e.what() << std::endl;
            return Return{false, "Error occurred"};
        }
    }
    
    virtual Return doExecute(const Args& args) = 0;
};
```

## Testing Command Pattern

### Run Test
```bash
./bin/test_command_demo
# Tests:
# - Simple command execution
# - Multiple commands
# - Parameter passing
# - Result handling
```

### Example Test
```cpp
#include "ICommand.hpp"

struct Args { int value; };
struct Result { int doubled; };

class DoubleCommand : public ICommand<Args, Result> {
    Result execute(const Args& args) override {
        return Result{args.value * 2};
    }
};

int main() {
    DoubleCommand cmd;
    
    Result result = cmd.execute(Args{5});
    assert(result.doubled == 10);  // ✅ Correct
    
    result = cmd.execute(Args{0});
    assert(result.doubled == 0);   // ✅ Edge case
    
    return 0;
}
```

## Integration with Other Patterns

### Command + Factory
```cpp
// Create commands by name
Factory<ICommand<Args, Result>>& factory = Factory::getInstance();
ICommand<Args, Result>* cmd = factory.create("QueryCommand");
```

### Command + ThreadPool
```cpp
// Execute commands in thread pool
ThreadPool pool(4);
pool.enqueue([cmd, args]() {
    Result result = cmd->execute(args);
});
```

### Command + Singleton
```cpp
// Global command factory
CommandFactory& factory = CommandFactory::getInstance();
ICommand<Args, Result>* cmd = factory.create("DatabaseQuery");
```

## Common Use Cases

| Use Case | Implementation |
|----------|----------------|
| Task queuing | CommandQueue with FIFO |
| RPC invocation | Command with serialized args |
| Undo/redo | UndoableCommand with history |
| Scheduled tasks | Command with delay/interval |
| Macros | CompositeCommand |
| Transactions | Command that bundles operations |

## Performance Considerations

- **Creation**: Negligible - just object allocation
- **Execution**: Depends on command logic
- **Memory**: Each command stores its parameters
- **Queueing**: Linear with queue size

## Thread Safety

```cpp
// Individual commands should be thread-safe
class ThreadSafeCommand : public ICommand<Args, Result> {
    Result execute(const Args& args) override {
        std::lock_guard<std::mutex> lock(mutex);
        // Mutually exclusive access to shared resources
        return doWork(args);
    }
private:
    std::mutex mutex;
};
```

## Summary

| Aspect | Command Pattern |
|--------|-----------------|
| **Use When** | Need to parameterize, queue, or log operations |
| **Avoids** | Tight coupling between invoker and executor |
| **Key Benefit** | Flexible, extensible request encapsulation |
| **Complexity** | Low to Medium |
| **Phase Usage** | 1 (ThreadPool), 3 (RPC invocation) |

---

**Location**: `design_patterns/command/`  
**Status**: ✅ Fully implemented and tested  
**Used By**: ThreadPool task execution, Phase 3 RPC framework
