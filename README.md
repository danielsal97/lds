# LDS — Local Drive Storage

A distributed NAS (Network Attached Storage) system built in **C++20** on Linux.  
From the user's perspective it is a regular block device. Under the hood, every block is stored across distributed storage nodes with RAID01 redundancy.

---

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│  User process: read() / write() on /dev/nbd0            │
└────────────────────────┬────────────────────────────────┘
                         │ VFS → NBD kernel driver
                         ▼
┌─────────────────────────────────────────────────────────┐
│  LDS Master Process (C++20)                             │
│                                                         │
│  ┌──────────┐   ┌───────────────┐   ┌───────────────┐  │
│  │  Reactor │──▶│ InputMediator │──▶│ LocalStorage  │  │
│  │  (epoll) │   │  (dispatcher) │   │  (in-memory)  │  │
│  └──────────┘   └───────────────┘   └───────────────┘  │
│       │                                                 │
│  ┌────┴──────────────────────────────────────────────┐  │
│  │  Plugin System (PNP + DirMonitor + dlopen)        │  │
│  └───────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
         │ TCP (Phase 2A)           │ UDP (Phase 2+)
         ▼                          ▼
┌─────────────────┐       ┌─────────────────────────────┐
│  Remote Client  │       │  Minion Nodes (Raspberry Pi) │
│  (Mac / Linux)  │       │  RAID01 block distribution   │
└─────────────────┘       └─────────────────────────────┘
```

### Two Modes

| Mode | Interface | Use case |
|---|---|---|
| **NBD** | Linux kernel block device `/dev/nbd0` | Local Linux block storage |
| **TCP** | Custom binary protocol over TCP | Remote client (Mac ↔ Linux) |

Both modes use the same `InputMediator` → `LocalStorage` pipeline. `TCPDriverComm` is a drop-in replacement for `NBDDriverComm` — they both implement `IDriverComm`.

---

## Components Built

### Phase 1 — Core Framework

| Component | Path | Description |
|---|---|---|
| **Reactor** | `design_patterns/reactor/` | `epoll`-based event loop — dispatches I/O events to handlers |
| **InputMediator** | `services/mediator/` | Reads `DriverData`, routes READ/WRITE/FLUSH/TRIM to storage |
| **LocalStorage** | `services/local_storage/` | In-memory block storage backend (`pread`/`pwrite` semantics) |
| **NBDDriverComm** | `services/communication_protocols/nbd/` | Reads `nbd_request` from kernel, writes `nbd_reply` back |
| **Factory** | `design_patterns/factory/` | Creates commands and plugins by name at runtime |
| **Observer** | `design_patterns/observer/` | `Dispatcher<T>` + `CallBack<T,Sub>` — type-safe event routing |
| **Command** | `design_patterns/command/` | `ICommand` interface with priority (WRITE > READ > FLUSH) |
| **ThreadPool + WPQ** | `utilities/threading/` | Priority work queue + fixed thread pool for async execution |
| **Plugin System** | `plugins/` | `DirMonitor` (inotify) + `PNP` + `soLoader` — loads `.so` plugins at runtime via `dlopen` |
| **Logger** | `utilities/logger/` | Thread-safe singleton logger |

### Phase 2A — TCP Client Bridge

| Component | Path | Description |
|---|---|---|
| **TCPDriverComm** | `services/communication_protocols/tcp/` | TCP server — same `IDriverComm` interface as NBD, remote clients read/write blocks |
| **Interfaces** | `interfaces/` | Shared `IDriverComm`, `IMediator`, `IStorage` headers |
| **TCP client** | `test/integration/test_tcp_client.py` | Python client for end-to-end testing |

---

## Design Patterns

| Pattern | Where | Why |
|---|---|---|
| **Reactor** | `Reactor` + epoll | Single thread handles all I/O events without blocking |
| **Observer** | `Dispatcher<T>` / `CallBack<T>` | Decoupled event routing; templates avoid virtual call overhead |
| **Factory** | `CommandFactory` | Creates the right command type at runtime without `if/else` chains |
| **Command** | `ICommand` | Encapsulates work items; enables priority queue ordering |
| **Singleton** | `Logger` | Thread-safe single instance via C++11 magic statics |
| **Strategy** | `IDriverComm` | Swap NBD ↔ TCP without changing `InputMediator` |
| **Plugin** | `DirMonitor` + `dlopen` | Add features at runtime without recompiling the core |

---

## Build

**Requirements:** GCC 10+ (C++20), Linux, `make`, `libnbd-dev` (for NBD mode)

```bash
# Build everything (library + all binaries + tests):
make

# Build only the app:
make app

# Run all unit tests:
make run_tests

# Clean:
make clean
```

Output binaries are in `bin/`. The shared library is in `lib/libfoo-debug.so`.

---

## Run

### TCP Mode (no kernel modules needed — works on any Linux)

```bash
# Terminal 1 — start the server (128 MB storage on port 9999):
./bin/LDS tcp 9999 134217728

# Terminal 2 — run the Python client:
python3 test/integration/test_tcp_client.py --host 127.0.0.1 --port 9999

# Or connect from a remote machine (e.g. Mac via Tailscale):
python3 test/integration/test_tcp_client.py --host 100.x.x.x --port 9999
```

### NBD Mode (Linux only, requires root)

```bash
# Load the NBD kernel module:
sudo modprobe nbd max_part=8

# Start the server:
sudo ./bin/LDS nbd /dev/nbd0 134217728

# Mount as a filesystem (in another terminal):
sudo mkfs.ext4 /dev/nbd0
sudo mount /dev/nbd0 /mnt/lds
```

---

## TCP Wire Protocol

Requests and responses are big-endian binary frames:

```
Request (24 bytes header + optional payload):
  [type   : uint32 BE]   0=READ 1=WRITE 2=DISCONNECT 3=FLUSH 4=TRIM
  [handle : uint64 BE]   unique request ID for matching responses
  [offset : uint64 BE]   byte offset into block device
  [length : uint32 BE]   payload length in bytes
  [data   : length bytes] (WRITE only)

Response (16 bytes header + optional payload):
  [handle : uint64 BE]   matches request handle
  [error  : uint32 BE]   0 = success
  [length : uint32 BE]   response payload length
  [data   : length bytes] (READ only)
```

---

## Project Structure

```
lds/
├── app/                    # Entry point — LDS.cpp (nbd/tcp dual mode)
├── interfaces/             # Shared interfaces: IDriverComm, IMediator, IStorage
├── design_patterns/
│   ├── reactor/            # epoll event loop
│   ├── observer/           # Dispatcher + CallBack templates
│   ├── factory/            # Plugin/command factory
│   ├── command/            # ICommand interface
│   └── singleton/          # Thread-safe singleton
├── services/
│   ├── communication_protocols/
│   │   ├── nbd/            # NBD kernel driver interface
│   │   └── tcp/            # TCP server (IDriverComm drop-in)
│   ├── local_storage/      # In-memory block storage
│   └── mediator/           # InputMediator — routes events to storage
├── plugins/                # DirMonitor (inotify) + PNP + soLoader
├── utilities/
│   ├── logger/             # Thread-safe logger
│   └── threading/          # ThreadPool + WPQ (priority work queue)
├── test/
│   ├── unit/               # Unit tests (one binary per component)
│   └── integration/        # test_tcp_client.py — end-to-end TCP test
└── external/               # inotify C++ wrapper
```

---

## Key Technical Decisions

**Why epoll + Reactor?** A single thread handles all I/O — no thread-per-connection overhead. The Reactor dispatches each fd event to its registered handler and returns to `epoll_wait` immediately.

**Why UDP for minion communication?** Block storage operations are naturally fire-and-forget. The application layer handles retries with exponential backoff and MSG_ID matching — avoids TCP's connection overhead for high-frequency small messages.

**Why `TCPDriverComm` implements `IDriverComm`?** The mediator, thread pool, and storage pipeline know nothing about the transport. Swapping NBD for TCP requires changing only `main()`.

**Why templates for Observer/Factory?** Avoids virtual dispatch overhead in the hot path. `Dispatcher<T>` is instantiated at compile time for each event type.

---

## Technologies

C++20 · Linux · epoll · inotify · NBD (Network Block Device) · TCP/UDP sockets · pthreads · dlopen/dlsym · POSIX · GNU Make · Docker · Python 3

---

## Roadmap

- [x] Phase 1 — Core framework (Reactor, NBD, plugin system, mediator)
- [x] Phase 2A — TCP bridge (remote Mac ↔ Linux block I/O)
- [ ] Phase 2 — RAID01 distribution (MinionProxy, ResponseManager, Scheduler)
- [ ] Phase 3 — Reliability (Watchdog, AutoDiscovery)
- [ ] Phase 4 — Minion server (Raspberry Pi storage nodes)
