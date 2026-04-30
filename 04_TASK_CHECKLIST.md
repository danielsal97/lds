# LDS Implementation Task Checklist

## Phase 1: Core Framework Integration (Week 1-2)

### Task 1.1: InputMediator ⏱️ 4 hours
- [ ] Create `services/mediator/include/InputMediator.hpp`
- [ ] Create `services/mediator/src/InputMediator.cpp`
- [ ] Implement `OnNBDEvent()` method
- [ ] Parse NBD DriverData.type (READ/WRITE/FLUSH/TRIM)
- [ ] Create appropriate ICommand (ReadCommand, WriteCommand, etc.)
- [ ] Push command to ThreadPool.AddCommand()
- [ ] Wire Reactor callback to call InputMediator
- [ ] Write unit tests
  - [ ] Test READ event → ReadCommand creation
  - [ ] Test WRITE event → WriteCommand creation
  - [ ] Test priority handling
  - [ ] Verify commands in WPQ
- [ ] Compile without errors
- [ ] All tests pass ✓

### Task 1.2: ReadCommand ⏱️ 6 hours
- [ ] Create `services/commands/include/ReadCommand.hpp`
- [ ] Create `services/commands/src/ReadCommand.cpp`
- [ ] Inherit from ICommand with MEDIUM priority
- [ ] Store: offset_ (uint64_t), length_ (uint32_t)
- [ ] Implement Execute() method
  - [ ] Get block location from RAID01Manager
  - [ ] Send GET_BLOCK to minion 1 via MinionProxy
  - [ ] Wait for response via ResponseManager
  - [ ] If success: return data, done
  - [ ] If timeout/fail: try minion 2 (replica)
  - [ ] If both fail: return error
- [ ] Write unit tests
  - [ ] Mock RAID01Manager.GetBlockLocation()
  - [ ] Mock MinionProxy.SendGetBlock()
  - [ ] Fake successful minion response
  - [ ] Test fallback to replica
  - [ ] Test both minions fail
- [ ] Compile without errors
- [ ] All tests pass ✓

### Task 1.3: WriteCommand ⏱️ 6 hours
- [ ] Create `services/commands/include/WriteCommand.hpp`
- [ ] Create `services/commands/src/WriteCommand.cpp`
- [ ] Inherit from ICommand with MEDIUM priority
- [ ] Store: offset_ (uint64_t), data_ (Buffer)
- [ ] Implement Execute() method
  - [ ] Get 2 block locations from RAID01Manager (RAID01!)
  - [ ] Send PUT_BLOCK to minion1 via MinionProxy
  - [ ] Send PUT_BLOCK to minion2 via MinionProxy
  - [ ] Wait for both responses
  - [ ] Success if ≥1 minion succeeded (RAID01 requirement)
  - [ ] Return error if both failed
- [ ] Write unit tests
  - [ ] Mock RAID01Manager.GetBlockLocation()
  - [ ] Mock MinionProxy.SendPutBlock()
  - [ ] Test both minions succeed
  - [ ] Test one minion fails
  - [ ] Test both minions fail
  - [ ] Verify RAID01 2-copy requirement
- [ ] Compile without errors
- [ ] All tests pass ✓

### Task 1.4: Register Commands with Factory ⏱️ 2 hours
- [ ] Update main app initialization code
- [ ] Get Factory singleton instance
- [ ] Register READ command
  ```cpp
  factory.Add("READ", []() { 
    return std::make_shared<ReadCommand>(); 
  });
  ```
- [ ] Register WRITE command
- [ ] Register FLUSH command (stub ok for now)
- [ ] Test Factory.Create("READ") works
- [ ] Test Factory.Create("WRITE") works
- [ ] Compile without errors
- [ ] Factory unit tests pass ✓

### Phase 1 Summary
- [ ] InputMediator created and tested
- [ ] ReadCommand created and tested
- [ ] WriteCommand created and tested
- [ ] Registered with Factory
- [ ] Integration test: NBD event → Command creation → ThreadPool
- **Milestone 1 Achieved:** Components wire together ✓

---

## Phase 2: Data Management (Week 2-3)

### Task 2.1: RAID01 Manager ⏱️ 12 hours
- [ ] Create `services/storage/include/RAID01Manager.hpp`
- [ ] Create `services/storage/src/RAID01Manager.cpp`
- [ ] Create Minion struct
  - [ ] int id
  - [ ] std::string ip
  - [ ] int port
  - [ ] Status enum (HEALTHY, DEGRADED, FAILED)
  - [ ] time_t last_response_time
- [ ] Implement AddMinion(int id, const string& ip, int port)
- [ ] Implement GetBlockLocation(uint64_t block_num) → pair<int,int>
  - [ ] Algorithm: minion1 = block_num % count
  - [ ] minion2 = (block_num + 1) % count
  - [ ] Skip failed minions
- [ ] Implement FailMinion(int id)
  - [ ] Mark status = FAILED
  - [ ] Notify Scheduler to reroute
- [ ] Implement persistence
  - [ ] SaveMapping(const string& filepath)
  - [ ] LoadMapping(const string& filepath)
- [ ] Write unit tests
  - [ ] Test block mapping with 3 minions
  - [ ] Test block mapping with 4 minions
  - [ ] Test FailMinion rerouting
  - [ ] Test save/load persistence
  - [ ] Test all blocks accessible
- [ ] Create integration test
  - [ ] Real commands using GetBlockLocation()
- [ ] Compile without errors
- [ ] All tests pass ✓

### Task 2.2: MinionProxy ⏱️ 14 hours
- [ ] Create `services/network/include/MinionProxy.hpp`
- [ ] Create `services/network/src/MinionProxy.cpp`
- [ ] Implement UDP socket
  - [ ] Create UDP socket
  - [ ] Store minion addresses (IP, port)
- [ ] Implement message serialization
  - [ ] Format: [MSG_ID(4B)][OP(1B)][OFFSET(8B)][LEN(4B)][DATA(var)]
  - [ ] MSG_ID should be unique per request
  - [ ] OP: 0x01=GET, 0x02=PUT
- [ ] Implement SendGetBlock()
  - [ ] Create GET message
  - [ ] Serialize it
  - [ ] Send to minion via UDP
  - [ ] Return MSG_ID (don't wait for response!)
- [ ] Implement SendPutBlock()
  - [ ] Create PUT message with data
  - [ ] Serialize it
  - [ ] Send to minion via UDP
  - [ ] Return MSG_ID
- [ ] Write unit tests
  - [ ] Test message format is correct
  - [ ] Test serialization (data → bytes)
  - [ ] Test with FakeMinion (in-memory)
  - [ ] Verify MSG_ID uniqueness
  - [ ] Test multiple concurrent sends
- [ ] Create integration test
  - [ ] Real minion server receives and responds
- [ ] Compile without errors
- [ ] All tests pass ✓

### Task 2.3: ResponseManager ⏱️ 10 hours
- [ ] Create `services/network/include/ResponseManager.hpp`
- [ ] Create `services/network/src/ResponseManager.cpp`
- [ ] Implement background receiver thread
  - [ ] Listen on UDP port for responses
  - [ ] Parse response messages
  - [ ] Extract MSG_ID, status, data
- [ ] Implement RegisterCallback()
  - [ ] Map MSG_ID → callback function
  - [ ] Thread-safe (use mutex)
- [ ] Implement response handling
  - [ ] When response arrives, call registered callback
  - [ ] Handle out-of-order responses
- [ ] Implement Start() / Stop()
  - [ ] Start background thread
  - [ ] Stop thread gracefully
- [ ] Write unit tests
  - [ ] Test callback registration
  - [ ] Test callback invocation on response
  - [ ] Test multiple concurrent responses
  - [ ] Test out-of-order handling
  - [ ] Test thread safety
- [ ] Compile without errors
- [ ] All tests pass ✓

### Task 2.4: Scheduler ⏱️ 10 hours
- [ ] Create `services/execution/include/Scheduler.hpp`
- [ ] Create `services/execution/src/Scheduler.cpp`
- [ ] Implement request tracking
  - [ ] Map MSG_ID → request info
  - [ ] Store deadline (now + timeout)
- [ ] Implement OnResponse(MSG_ID, response)
  - [ ] Find request by MSG_ID
  - [ ] Mark complete with response
  - [ ] Notify waiting thread
- [ ] Implement WaitForResponse(MSG_ID, timeout_ms)
  - [ ] Create request entry
  - [ ] Wait for response (with timeout)
  - [ ] Return response or timeout error
  - [ ] Clean up after
- [ ] Implement retry logic
  - [ ] On timeout: retry_delay = 100ms
  - [ ] Each retry: retry_delay *= 2 (cap at 5000ms)
  - [ ] Max retries: 3
  - [ ] Give up after
- [ ] Write unit tests
  - [ ] Test successful response
  - [ ] Test timeout (no response)
  - [ ] Test exponential backoff delays
  - [ ] Test max retries exceeded
  - [ ] Test multiple concurrent requests
- [ ] Compile without errors
- [ ] All tests pass ✓

### Phase 2 Summary
- [ ] RAID01Manager implemented and tested
- [ ] MinionProxy network communication working
- [ ] ResponseManager async handling working
- [ ] Scheduler retry logic working
- [ ] Integration: Command → Network → Minion → Response
- **Milestone 2 Achieved:** Network communication works ✓

---

## Phase 3: Reliability Features (Week 3-4)

### Task 3.1: Watchdog ⏱️ 8 hours
- [ ] Create `services/health/include/Watchdog.hpp`
- [ ] Create `services/health/src/Watchdog.cpp`
- [ ] Implement health tracking
  - [ ] Track last_response_time for each minion
  - [ ] Track status (HEALTHY, DEGRADED, FAILED)
- [ ] Implement background monitor thread
  - [ ] Run every 5 seconds
  - [ ] Check each minion's health
- [ ] Implement PING mechanism
  - [ ] Send PING message to each minion
  - [ ] Record response time
  - [ ] Update last_response_time
- [ ] Implement failure detection
  - [ ] If no response for 15 seconds → FAILED
  - [ ] Mark in RAID01Manager
  - [ ] Notify other components
- [ ] Implement recovery detection
  - [ ] If previously FAILED minion responds → HEALTHY
  - [ ] Restart using that minion
- [ ] Write unit tests
  - [ ] Test health tracking
  - [ ] Test 15-second failure threshold
  - [ ] Test recovery detection
  - [ ] Test PING sending
  - [ ] Mock time for deterministic testing
- [ ] Compile without errors
- [ ] All tests pass ✓

### Task 3.2: Auto-Discovery ⏱️ 10 hours
- [ ] Create `services/discovery/include/AutoDiscovery.hpp`
- [ ] Create `services/discovery/src/AutoDiscovery.cpp`
- [ ] Implement broadcast listener
  - [ ] Listen for minion broadcasts on UDP
  - [ ] Parse minion info (ID, IP, port)
- [ ] Implement new minion detection
  - [ ] If unknown minion broadcasts
  - [ ] Register with RAID01Manager.AddMinion()
  - [ ] Start data rebalancing
- [ ] Implement minion rejoin detection
  - [ ] If previously FAILED minion broadcasts
  - [ ] Mark as HEALTHY in RAID01Manager
  - [ ] Resync missing data
- [ ] Implement data rebalancing
  - [ ] Find blocks that need copying to new minion
  - [ ] Create rebalance commands
  - [ ] Copy via MinionProxy
  - [ ] Verify copies completed
- [ ] Write unit tests
  - [ ] Test new minion discovery
  - [ ] Test registration with RAID01Manager
  - [ ] Test minion rejoin
  - [ ] Test data rebalancing
- [ ] Compile without errors
- [ ] All tests pass ✓

### Task 3.3: Error Handling & Logging ⏱️ 6 hours
- [ ] Review all components for error handling
- [ ] Add comprehensive logging
  - [ ] Log all READ/WRITE operations
  - [ ] Log all errors
  - [ ] Log minion status changes
  - [ ] Use existing Logger component
- [ ] Verify graceful failure modes
  - [ ] Network errors handled
  - [ ] Minion failures handled
  - [ ] Timeouts handled
- [ ] Test error scenarios
  - [ ] Network partition
  - [ ] Minion crash
  - [ ] Storage full
  - [ ] Invalid messages
- [ ] Compile without errors
- [ ] All error tests pass ✓

### Phase 3 Summary
- [ ] Watchdog monitoring minion health
- [ ] Auto-Discovery finding new minions
- [ ] Error handling graceful
- [ ] Comprehensive logging
- **Milestone 3 Achieved:** Fault tolerance working ✓

---

## Phase 4: Minion-Side Implementation (Week 4)

### Task 4.1: Minion Server ⏱️ 12 hours
- [ ] Create `minion/include/MinionServer.hpp`
- [ ] Create `minion/src/MinionServer.cpp`
- [ ] Create `minion/src/minion_main.cpp`
- [ ] Implement UDP server
  - [ ] Listen on port (arg from CLI)
  - [ ] Receive messages from master
  - [ ] Parse message format
- [ ] Implement local storage
  - [ ] Simple map<uint64_t, Buffer> for blocks
  - [ ] Load from disk on startup (if file exists)
  - [ ] Save to disk on shutdown
  - [ ] Or: SQLite database
- [ ] Implement GET_BLOCK handler
  - [ ] Parse offset, length
  - [ ] Read from local storage
  - [ ] Create response message
  - [ ] Send back to master with MSG_ID
- [ ] Implement PUT_BLOCK handler
  - [ ] Parse offset, data
  - [ ] Store in local storage
  - [ ] Send success response to master
- [ ] Implement DELETE_BLOCK handler
  - [ ] Parse offset
  - [ ] Delete from local storage
  - [ ] Send success response
- [ ] Implement PING handler
  - [ ] Respond with PONG
  - [ ] Used by Watchdog for health checks
- [ ] Write unit tests
  - [ ] Test GET_BLOCK
  - [ ] Test PUT_BLOCK
  - [ ] Test persistence
  - [ ] Test message parsing
- [ ] Create minion executable
  - [ ] Usage: `./minion_server <port>`
  - [ ] Listens and processes requests
- [ ] Compile without errors
- [ ] All tests pass ✓

### Phase 4 Summary
- [ ] Minion server fully implemented
- [ ] Handles GET/PUT/DELETE from master
- [ ] Persistent storage working
- [ ] Can be tested with real master
- **Milestone 4 Achieved:** Minion side working ✓

---

## Phase 5: Integration & Testing (Week 5)

### Task 5.1: End-to-End Integration ⏱️ 16 hours
- [ ] Start minion server: `./minion_server 9001`
- [ ] Start another minion: `./minion_server 9002`
- [ ] Update main app to use real minions
  - [ ] Configure RAID01Manager with minion addresses
  - [ ] Set up NBD device
  - [ ] Run master server
- [ ] Test simple READ
  - [ ] Write test data to minion
  - [ ] User reads via NBD
  - [ ] Verify correct data returned
- [ ] Test simple WRITE
  - [ ] User writes via NBD
  - [ ] Verify 2 copies on minions (RAID01)
- [ ] Test concurrent operations
  - [ ] 5 concurrent READs
  - [ ] 5 concurrent WRITEs
  - [ ] Mixed READ/WRITE
- [ ] Test minion failure
  - [ ] Kill minion 1
  - [ ] READ should use minion 2 (replica)
  - [ ] User still gets data
- [ ] Test data integrity
  - [ ] Write known pattern
  - [ ] Kill minion 1
  - [ ] Read back
  - [ ] Verify pattern intact
- [ ] Create comprehensive test suite
  - [ ] Use gtest for integration tests

### Task 5.2: Unit Tests ⏱️ 20 hours
- [ ] Create test files:
  - [ ] `test/unit/test_raid01_manager.cpp`
  - [ ] `test/unit/test_minion_proxy.cpp`
  - [ ] `test/unit/test_response_manager.cpp`
  - [ ] `test/unit/test_scheduler.cpp`
  - [ ] `test/unit/test_read_command.cpp`
  - [ ] `test/unit/test_write_command.cpp`
  - [ ] `test/unit/test_watchdog.cpp`
  - [ ] `test/unit/test_auto_discovery.cpp`
- [ ] Write test cases for each component
- [ ] Use mocks and fakes
- [ ] Achieve >80% code coverage
- [ ] All tests pass

### Task 5.3: Integration Tests ⏱️ 20 hours
- [ ] Create test files:
  - [ ] `test/integration/test_read_write.cpp`
  - [ ] `test/integration/test_fault_tolerance.cpp`
  - [ ] `test/integration/test_minion_failure.cpp`
  - [ ] `test/integration/test_auto_discovery.cpp`
  - [ ] `test/integration/test_concurrent_ops.cpp`
- [ ] Test scenarios:
  - [ ] Normal READ/WRITE
  - [ ] Single minion failure
  - [ ] Two minion failure (degradation)
  - [ ] Minion recovery
  - [ ] Concurrent operations
  - [ ] Large data transfers
- [ ] All tests pass

### Task 5.4: System Tests ⏱️ 12 hours
- [ ] Set up 3-4 Raspberry Pis
- [ ] Deploy minion software to each
- [ ] Configure master for real hardware
- [ ] Run all scenarios:
  - [ ] Basic operations
  - [ ] Failure scenarios
  - [ ] Recovery scenarios
  - [ ] Stress testing
- [ ] Measure performance:
  - [ ] Latency
  - [ ] Throughput
  - [ ] Data integrity
- [ ] Document results

### Phase 5 Summary
- [ ] All components integrated
- [ ] Comprehensive test coverage
- [ ] System tested on real hardware
- [ ] Performance measured
- **Milestone 5 Achieved:** Full system working ✓

---

## Phase 6: Optimization & Polish (Week 6)

### Task 6.1: Performance Optimization ⏱️ 10 hours
- [ ] Profile code (gprof, perf)
- [ ] Identify slow operations
- [ ] Optimize hot paths
- [ ] Batch operations where possible
- [ ] Tune thread count
- [ ] Tune timeout values
- [ ] Measure improvements

### Task 6.2: Documentation ⏱️ 8 hours
- [ ] Code documentation
  - [ ] Comments on complex logic
  - [ ] API documentation
- [ ] Deployment guide
  - [ ] How to set up minions
  - [ ] How to configure master
  - [ ] How to run tests
- [ ] Troubleshooting guide
  - [ ] Common issues
  - [ ] How to debug
  - [ ] Log file locations
- [ ] Architecture overview (updated)

### Task 6.3: CI/CD & Build ⏱️ 8 hours
- [ ] Set up CMake build
- [ ] Configure CI/CD pipeline
- [ ] Automated testing on commits
- [ ] Code coverage reporting
- [ ] Build documentation
- [ ] Docker containers for deployment

### Phase 6 Summary
- [ ] Performance optimized
- [ ] Fully documented
- [ ] CI/CD pipeline ready
- **Milestone 6 Achieved:** Production ready ✓

---

## 🎯 Quick Stats

| Phase | Duration | Tasks | Est. Hours |
|-------|----------|-------|-----------|
| 1 | 2 weeks | 4 | 18 |
| 2 | 2 weeks | 4 | 46 |
| 3 | 1 week | 3 | 24 |
| 4 | 1 week | 1 | 12 |
| 5 | 1 week | 4 | 68 |
| 6 | 1 week | 3 | 26 |
| **TOTAL** | **~8 weeks** | **19** | **194 hours** |

**Average: ~24 hours/week or ~5 hours/day for 8 weeks**

---

## ✅ Success Criteria

- [ ] All tasks completed
- [ ] All unit tests passing (>80% coverage)
- [ ] All integration tests passing
- [ ] System tests on real hardware passing
- [ ] Performance meets requirements
  - [ ] Latency < 100ms
  - [ ] Throughput > 50MB/s
- [ ] Comprehensive documentation
- [ ] Zero data loss with RAID01
- [ ] Graceful failure handling
- [ ] Ready for production deployment

---

## 📝 Notes

- Update this checklist as you progress
- Check off items as they're completed
- Mark any blockers or issues
- Track actual vs estimated time
- Use for status reporting

**Good luck! 🚀**
