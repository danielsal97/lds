# LDS Implementation Roadmap

## 🎯 Goal
Complete a fully functional IoT-based NAS/RAID01 Drive System with master-minion architecture.

## 📊 Current Status

### ✅ Already Implemented
- [x] Reactor (epoll-based event handling)
- [x] ThreadPool (worker threads with WPQ)
- [x] WPQ (Waitable Priority Queue)
- [x] ICommand (abstract command interface)
- [x] Factory Pattern (generic object creation)
- [x] Plugin System (PNP for dynamic loading)
- [x] NBD Driver Communication (block device interface)
- [x] Logger (event logging)
- [x] Design Pattern implementations

### ❌ Missing/Incomplete
- [ ] InputMediator
- [ ] ReadCommand & WriteCommand
- [ ] RAID01 Manager
- [ ] MinionProxy (network communication)
- [ ] ResponseManager (async responses)
- [ ] Scheduler (retry logic)
- [ ] Watchdog (health monitoring)
- [ ] Auto-Discovery (dynamic minion discovery)
- [ ] Minion-side implementation
- [ ] Integration & testing

---

## 🏗️ Implementation Phases

### Phase 1: Core Framework Integration (Week 1-2)
**Goal:** Wire existing components together

#### Task 1.1: InputMediator
**Effort:** 4 hours
**Files to Create:**
- `services/mediator/include/InputMediator.hpp`
- `services/mediator/src/InputMediator.cpp`

**What to Do:**
```
1. Create InputMediator class
2. Convert NBD DriverData events to ICommand objects
3. Wire Reactor's handler to call InputMediator
4. Add commands to ThreadPool's WPQ
```

**Code Structure:**
```cpp
class InputMediator {
public:
    explicit InputMediator(ThreadPool& pool);
    void OnNBDEvent(const DriverData& request);
private:
    ThreadPool& thread_pool_;
    Factory<ICommand, std::string>& factory_;
};
```

**Testing:**
- Mock NBD events
- Verify commands added to WPQ
- Check priority handling

**Success Criteria:**
- ✓ Reactor calls InputMediator on each NBD event
- ✓ Commands queued in correct priority order
- ✓ Unit tests pass

---

#### Task 1.2: ReadCommand Implementation
**Effort:** 6 hours
**Files to Create:**
- `services/commands/include/ReadCommand.hpp`
- `services/commands/src/ReadCommand.cpp`

**What to Do:**
```
1. Inherit from ICommand
2. Store offset, length parameters
3. Implement Execute() method
   - Query RAID01Manager for block locations
   - Send request to primary minion via MinionProxy
   - Wait for response via ResponseManager
   - On failure, retry with replica minion
   - Return data to user via NBD
```

**Dependencies:**
- ✓ ICommand (ready)
- ✓ RAID01Manager (needed - Task 2.1)
- ✓ MinionProxy (needed - Task 2.2)

**Testing:**
- Mock RAID01Manager
- Mock MinionProxy
- Fake minion responses
- Test fallback to replica

**Success Criteria:**
- ✓ Reads block from minion
- ✓ Falls back to replica on failure
- ✓ Returns correct data

---

#### Task 1.3: WriteCommand Implementation
**Effort:** 6 hours
**Files to Create:**
- `services/commands/include/WriteCommand.hpp`
- `services/commands/src/WriteCommand.cpp`

**What to Do:**
```
1. Inherit from ICommand
2. Store offset, data buffer
3. Implement Execute() method
   - Query RAID01Manager for 2 minion locations
   - Send data to BOTH minions (RAID01)
   - Wait for responses from both
   - Return success only if at least 1 copy succeeded
```

**Testing:**
- Test 2-copy writes
- Test single minion failure
- Test data integrity

**Success Criteria:**
- ✓ Data written to 2 minions
- ✓ Survives single minion failure
- ✓ Data integrity maintained

---

#### Task 1.4: Register Commands with Factory
**Effort:** 2 hours

**What to Do:**
```cpp
// In main app initialization:
auto& factory = Factory<ICommand, std::string>::Instance();
factory.Add("READ", []() { return std::make_shared<ReadCommand>(); });
factory.Add("WRITE", []() { return std::make_shared<WriteCommand>(); });
factory.Add("FLUSH", []() { return std::make_shared<FlushCommand>(); });
```

**Testing:**
- Factory can create READ commands
- Factory can create WRITE commands

---

### Phase 2: Data Management (Week 2-3)
**Goal:** Implement storage strategy and network communication

#### Task 2.1: RAID01 Manager
**Effort:** 12 hours
**Files to Create:**
- `services/storage/include/RAID01Manager.hpp`
- `services/storage/src/RAID01Manager.cpp`

**What to Do:**
```
1. Create minion registry (ID, IP, port, status)
2. Implement block-to-minion mapping algorithm
   - Each block stored on exactly 2 minions
   - Simple strategy: Block N → (N % num_minions, (N+1) % num_minions)
3. Handle failed minions
   - Mark minion as failed
   - Reroute to healthy minion
4. Persistence
   - Save mapping to disk
   - Load on startup
```

**Data Structures:**
```cpp
struct Minion {
    int id;
    std::string ip;
    int port;
    enum Status { HEALTHY, DEGRADED, FAILED };
    Status status;
    time_t last_response_time;
};

class RAID01Manager {
public:
    std::pair<int, int> GetBlockLocation(uint64_t block_num);
    void AddMinion(int id, const std::string& ip, int port);
    void FailMinion(int id);
    void SaveMapping(const std::string& filepath);
    void LoadMapping(const std::string& filepath);
private:
    std::map<int, Minion> minions_;
    std::map<uint64_t, std::pair<int, int>> block_map_;
};
```

**Testing:**
- Test block mapping calculation
- Test minion failure handling
- Test persistence (save/load)
- Test with 3, 4, 5 minions

**Success Criteria:**
- ✓ Correct block locations
- ✓ Handles minion failures
- ✓ Mapping survives restart

---

#### Task 2.2: MinionProxy
**Effort:** 14 hours
**Files to Create:**
- `services/network/include/MinionProxy.hpp`
- `services/network/src/MinionProxy.cpp`

**What to Do:**
```
1. UDP socket management
   - Create UDP socket
   - Store minion addresses
2. Message serialization
   - Convert data to binary format
   - Create message headers (MSG_ID, op type, offset, length)
3. Send methods
   - SendGetBlock(minion_id, offset, length) → returns MSG_ID
   - SendPutBlock(minion_id, offset, data) → returns MSG_ID
4. Async sending (fire-and-forget)
   - Don't wait for response
   - ResponseManager handles replies
```

**Message Format:**
```
Master → Minion:
[MSG_ID(4B)][OP_TYPE(1B)][OFFSET(8B)][LENGTH(4B)][DATA(var)]

Minion → Master:
[MSG_ID(4B)][STATUS(1B)][LENGTH(4B)][RESERVED(4B)][DATA(var)]
```

**Testing:**
- Test message serialization
- Test UDP sending
- Use Fake Minion for testing
- Verify message format

**Success Criteria:**
- ✓ Messages formatted correctly
- ✓ Sent to correct minion
- ✓ Minions receive and parse correctly

---

#### Task 2.3: ResponseManager
**Effort:** 10 hours
**Files to Create:**
- `services/network/include/ResponseManager.hpp`
- `services/network/src/ResponseManager.cpp`

**What to Do:**
```
1. UDP receiver thread
   - Listen on port for minion responses
   - Run in background thread
2. Message parsing
   - Extract MSG_ID from response
   - Parse status and data
3. Callback registration
   - RegisterCallback(MSG_ID, callback_function)
   - When response arrives, call callback
4. Timeout handling
   - Track when request was sent
   - Scheduler handles timeouts
```

**Testing:**
- Test message reception
- Test callback invocation
- Test timeout detection
- Test concurrent responses

**Success Criteria:**
- ✓ Receives minion responses
- ✓ Calls correct callback
- ✓ Thread-safe

---

#### Task 2.4: Scheduler
**Effort:** 10 hours
**Files to Create:**
- `services/execution/include/Scheduler.hpp`
- `services/execution/src/Scheduler.cpp`

**What to Do:**
```
1. Request tracking
   - Map MSG_ID to pending request
   - Store timeout deadline
2. Response handling
   - When ResponseManager gets response
   - Call Scheduler.OnResponse(MSG_ID, response)
   - Mark request complete
3. Timeout detection
   - Poll pending requests
   - If deadline passed, mark failed
4. Retry logic
   - Exponential backoff: 1s, 2s, 4s, max 10s
   - Retries up to 3 times
   - Then give up
```

**Testing:**
- Test successful response
- Test timeout and retry
- Test exponential backoff
- Test max retry limit

**Success Criteria:**
- ✓ Handles responses
- ✓ Retries on timeout
- ✓ Gives up after max retries

---

### Phase 3: Reliability Features (Week 3-4)
**Goal:** Implement monitoring, failover, and discovery

#### Task 3.1: Watchdog
**Effort:** 8 hours
**Files to Create:**
- `services/health/include/Watchdog.hpp`
- `services/health/src/Watchdog.cpp`

**What to Do:**
```
1. Health check thread
   - Run in background
   - Every 5 seconds: PING each minion
2. Health tracking
   - Record last response time
   - Calculate health status
3. Failure detection
   - If no response for 15 seconds → FAILED
   - Notify RAID01Manager
   - Trigger auto-discovery
4. Recovery detection
   - If previously-failed minion responds → HEALTHY
   - Resume using that minion
```

**Testing:**
- Test health tracking
- Test failure detection (15s threshold)
- Test recovery detection
- Mock minion failures

**Success Criteria:**
- ✓ Detects failed minions
- ✓ Notifies other components
- ✓ Detects recovery

---

#### Task 3.2: Auto-Discovery
**Effort:** 10 hours
**Files to Create:**
- `services/discovery/include/AutoDiscovery.hpp`
- `services/discovery/src/AutoDiscovery.cpp`

**What to Do:**
```
1. Broadcast listening
   - Listen for minion broadcasts on UDP
   - Parse: "Hello, I'm Minion-42, Port 9000"
2. New minion detection
   - If unknown minion broadcasts
   - Register with RAID01Manager
   - Start data rebalancing
3. Minion rejoin detection
   - If previously-failed minion broadcasts
   - Restart using that minion
   - Resync any missing data
4. Data rebalancing
   - Find blocks that need to be copied to new minion
   - Create rebalance tasks
   - Copy via Scheduler
```

**Testing:**
- Test minion discovery
- Test data rebalancing
- Test rejoin scenario

**Success Criteria:**
- ✓ Discovers new minions
- ✓ Rebalances data
- ✓ Recovers from failures

---

#### Task 3.3: Error Handling & Logging
**Effort:** 6 hours

**What to Do:**
```
1. Comprehensive logging
   - All operations logged (READ, WRITE, errors)
   - Log levels: DEBUG, INFO, WARN, ERROR
2. Error recovery
   - Graceful handling of network errors
   - Graceful handling of minion failures
   - Retry with fallback
3. Status reporting
   - Commands report success/failure
   - Return proper status codes to user
```

**Testing:**
- Test error scenarios
- Verify logging
- Check error propagation

---

### Phase 4: Minion-Side Implementation (Week 4)
**Goal:** Implement minion server that master communicates with

#### Task 4.1: Minion Server Framework
**Effort:** 12 hours
**Files to Create:**
- `minion/include/MinionServer.hpp`
- `minion/src/MinionServer.cpp`
- `minion/src/minion_main.cpp`

**What to Do:**
```
1. UDP server
   - Listen for master commands
   - Parse messages from master
2. Command handlers
   - GET_BLOCK: Read from local storage
   - PUT_BLOCK: Write to local storage
   - DELETE_BLOCK: Delete from local storage
3. Response sender
   - Send responses back to master
   - Include MSG_ID for matching
4. Local storage
   - Simple file-based or memory-based
   - Store blocks by block ID
```

**Implementation:**
```cpp
class MinionServer {
public:
    void Start(int port);
    void HandleMessage(const Message& msg);
private:
    std::map<uint64_t, Buffer> storage_;
    void OnGetBlock(const Message& msg);
    void OnPutBlock(const Message& msg);
};
```

**Testing:**
- Test GET_BLOCK response
- Test PUT_BLOCK storage
- Test message parsing
- Send commands from master

**Success Criteria:**
- ✓ Minion receives commands
- ✓ Responds correctly
- ✓ Stores data persistently

---

### Phase 5: Integration & Testing (Week 5)
**Goal:** Put it all together and verify everything works

#### Task 5.1: End-to-End Integration
**Effort:** 16 hours

**What to Do:**
```
1. Wire all components in main app
2. Test simple READ
   - User reads file
   - Reactor receives
   - InputMediator creates ReadCommand
   - ThreadPool executes
   - MinionProxy sends request
   - ResponseManager gets response
   - Data returned to user
3. Test simple WRITE
   - User writes file
   - RAID01: 2 copies sent
   - Both minions store
   - Success response
4. Test failure scenarios
   - Kill minion 1
   - System uses minion 2 replica
   - Data accessible
5. Test recovery
   - Restart minion 1
   - Auto-Discovery finds it
   - Rebalancing restores state
```

**Testing Checklist:**
- [ ] Single READ works
- [ ] Single WRITE works
- [ ] Concurrent READs work
- [ ] Concurrent WRITEs work
- [ ] One minion fails → READ uses replica
- [ ] One minion fails → WRITE uses single copy
- [ ] Both minions unavailable → proper error
- [ ] Minion rejoins → auto-discovery works
- [ ] Data survives restart

---

#### Task 5.2: Unit Tests
**Effort:** 20 hours
**Files to Create:**
- `test/unit/test_raid01_manager.cpp`
- `test/unit/test_minion_proxy.cpp`
- `test/unit/test_response_manager.cpp`
- `test/unit/test_scheduler.cpp`
- `test/unit/test_read_command.cpp`
- `test/unit/test_write_command.cpp`
- `test/unit/test_watchdog.cpp`

**Testing Framework:** Google Test (gtest)

**Test Coverage:**
- Each component has unit tests
- Use mocks and fakes
- Test success and failure paths

---

#### Task 5.3: Integration Tests
**Effort:** 20 hours
**Files to Create:**
- `test/integration/test_read_write.cpp`
- `test/integration/test_fault_tolerance.cpp`
- `test/integration/test_minion_failure.cpp`
- `test/integration/test_auto_discovery.cpp`

**Scenarios:**
1. Normal operation (read/write)
2. Single minion failure
3. Two minion failure (should degrade gracefully)
4. Minion recovery
5. Concurrent operations
6. Large data transfers

---

#### Task 5.4: System Tests
**Effort:** 12 hours

**What to Do:**
```
1. Physical test with real Raspberry Pis
2. Create 3-4 minion instances
3. Run through all scenarios
4. Verify data integrity
5. Check performance/latency
6. Stress test (many concurrent operations)
```

---

### Phase 6: Optimization & Polish (Week 6)
**Goal:** Make it production-ready

#### Task 6.1: Performance Optimization
**Effort:** 10 hours
- Profile code (identify slow parts)
- Optimize hot paths
- Batch operations where possible
- Tune thread count, timeouts

#### Task 6.2: Documentation
**Effort:** 8 hours
- Code documentation (comments)
- API documentation
- Deployment guide
- Troubleshooting guide

#### Task 6.3: CI/CD & Build
**Effort:** 8 hours
- Set up automated testing
- Build validation
- Code coverage reporting
- Continuous deployment

---

## 📈 Timeline Summary

| Phase | Duration | Key Deliverable |
|-------|----------|-----------------|
| Phase 1: Integration | 2 weeks | InputMediator, Commands, Wiring |
| Phase 2: Data Management | 2 weeks | RAID01, Network, Scheduler |
| Phase 3: Reliability | 1 week | Watchdog, Auto-Discovery |
| Phase 4: Minion Server | 1 week | Minion implementation |
| Phase 5: Integration Testing | 1 week | Full system testing |
| Phase 6: Polish | 1 week | Optimization, Docs |
| **Total** | **~8 weeks** | **Production-ready LDS** |

---

## 🎯 Milestones

### ✅ Milestone 1: Components Wire Together
**End of Phase 1**
- InputMediator wired to Reactor
- Commands execute via ThreadPool
- Basic READ/WRITE work (single minion)

### ✅ Milestone 2: Network Communication Works
**End of Phase 2**
- Master ↔ Minion communication
- Async response handling
- Retry logic

### ✅ Milestone 3: Fault Tolerance
**End of Phase 3**
- RAID01 working (2 copies)
- Failure detection and failover
- Auto-discovery

### ✅ Milestone 4: Full System Working
**End of Phase 5**
- All components integrated
- All tests passing
- Real hardware tested

### ✅ Milestone 5: Production Ready
**End of Phase 6**
- Performance optimized
- Fully documented
- CI/CD pipeline

---

## 🧪 Testing Strategy

### Unit Tests (Priority 1)
- RAID01Manager block mapping
- Command execution logic
- Message serialization

### Integration Tests (Priority 2)
- READ/WRITE workflows
- Fault tolerance scenarios
- Auto-discovery

### System Tests (Priority 3)
- Real hardware
- Performance testing
- Stress testing

---

## 💡 Key Success Factors

1. **Test-Driven:** Write tests before code
2. **Incremental:** Complete each task fully before moving to next
3. **Modular:** Components should be testable independently
4. **Documentation:** Keep design docs updated
5. **Failure Testing:** Deliberately test failure scenarios

---

## 🚀 Getting Started Right Now

### Week 1 - Start Here:
1. **Today (Day 1):** Create InputMediator
   - Understand Reactor callback
   - Parse NBD DriverData
   - Create ReadCommand/WriteCommand objects
   
2. **Day 2-3:** Implement ReadCommand
   - Create basic structure
   - Mock RAID01Manager and MinionProxy
   - Test with Fake minion
   
3. **Day 4:** Implement WriteCommand
   - Similar to ReadCommand
   - Test dual writes
   
4. **Day 5:** Register with Factory
   - Wire into InputMediator
   - Test basic READ/WRITE (with mocks)

### Quick Win (2-3 hours):
```bash
# Create structure
mkdir -p services/mediator/{include,src}
mkdir -p services/commands/{include,src}
mkdir -p services/network/{include,src}
mkdir -p services/storage/{include,src}

# Create main header files with class stubs
# Compile and verify structure
# Then implement gradually
```

---

## 📋 Dependency Graph

```
Reactor ✓
    ↓
InputMediator (1.1)
    ↓
ReadCommand (1.2) ← Requires → RAID01Manager (2.1) ← Requires → MinionProxy (2.2)
WriteCommand (1.3) ↓                                                 ↓
Factory ✓         Scheduler (2.4) ← Requires → ResponseManager (2.3)
    ↓                ↓                          ↓
ThreadPool ✓         ↓                          ↓
    ↓               Watchdog (3.1) ← Requires → AutoDiscovery (3.2)
   WPQ ✓                ↓
                    Minion Server (4.1)
                        ↓
                   Integration Tests (5.1-5.4)
```

**Start with:** InputMediator → ReadCommand/WriteCommand → RAID01Manager → MinionProxy → ResponseManager → Scheduler

---

## ✨ Expected Final State

When complete, you will have:
- ✅ Fully functional IoT-based NAS system
- ✅ RAID01 fault tolerance (survives 1 minion death)
- ✅ Automatic failover and recovery
- ✅ Dynamic minion discovery
- ✅ Scalable to 10+ minions
- ✅ 1TB+ storage capacity
- ✅ Production-ready with tests
- ✅ Comprehensive documentation
- ✅ Working examples and plugins

**This is a complete, professional-grade system ready for real-world use.**

---

## 📞 Questions While Implementing?

Refer to:
1. **PROJECT_BOOK.html** - Architecture and design
2. **Code examples** in this roadmap
3. **Existing components** in codebase
4. **Test patterns** from existing tests

Good luck! 🚀
