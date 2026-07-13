# LDS — Local Drive Storage

A **modular storage infrastructure** built in **C++20** that separates transport, execution, storage, and persistence into independent layers.

**The Problem:**
Tightly coupled storage systems are expensive to maintain and extend. Adding a new transport (TCP, RDMA, UCX), storage backend (Memory, File, Network), or persistence strategy (File, SQLite, Redis) typically requires modifying core logic. This causes complexity to compound and risk to increase.

**The Solution:**
LDS demonstrates a layered architecture where:
- **Storage logic** (RAIDStorage) never knows about backend implementations
- **New backends** are added as new implementations, not modifications
- **Transport** (NBD, TCP) is swappable without touching storage
- **Metadata persistence** is independent from data storage
- **Asynchronous execution** is decoupled from business logic
- **Dependencies** are injected, not created internally

**Current Implementation:** RAID01-inspired storage with Memory and File backends, dual-transport support (NBD kernel block device + multi-client TCP), and infrastructure-grade layering.

---

## Architecture Principles

These principles guide every layer of LDS:

| Principle | Application |
|-----------|-------------|
| **Single Responsibility** | RAIDStorage handles only RAID logic; StorageFactory handles backend creation; IMetadataStore handles persistence |
| **Interface-Driven Design** | Concrete implementations depend on abstract interfaces (IStorage, IStorageMinion, IMetadataStore, IDriverComm) |
| **Open/Closed Principle** | New backends (NetworkMinion, UCXMinion) and metadata stores (SQLiteMetadataStore, RedisMetadataStore) are added as new implementations, never modifying existing code |
| **Dependency Injection** | All dependencies are external; RAIDStorage is constructed with minions + metadata store, not self-instantiation |
| **Pluggable Backends** | Swap memory ↔ file ↔ network storage by changing factory, not RAIDStorage code |
| **Transport Abstraction** | NBD and TCP are interchangeable; adding UDP or custom protocols requires zero changes to storage layer |
| **Asynchronous Execution** | I/O handling stays non-blocking; storage work is delegated to ThreadPool (storage itself may block) |
| **Data ≠ Metadata** | Raw blocks live in minions; offset→size mappings live in metadata store; independent persistence strategies |

---

## Architecture

### Initialization Phase

Dependencies are created once at startup and injected into components. This decouples business logic from infrastructure decisions.

```
main()
  │
  ├─ StorageFactory::CreateMinions(backend, path, count, size)
  │    └─ Returns std::vector<IStorageMinion> (Memory/File/Network)
  │
  ├─ MetadataFactory::CreateMetadataStore(backend, path)
  │    └─ Returns std::unique_ptr<IMetadataStore> (Memory/File)
  │
  ├─ RAIDStorage(minions, metadata_store)
  │    └─ No knowledge of backend types; only interfaces
  │
  └─ RunServer(driver, storage, reactor)
       └─ Reactor, InputMediator, ThreadPool created
```

**Why two factories?**
- Storage decisions (backend minion type) are separate from persistence decisions (how to store metadata)
- Each can evolve independently
- Factory choices happen once; RAIDStorage is immutable after construction

---

### Runtime Phase

Requests flow through multiple abstraction layers without blocking.

```
Client Request (NBD/TCP)
    │
    ▼
Reactor (epoll)
    │ detects client fd ready
    ▼
InputMediator::Notify(fd)
    │ dispatcher routes to handler
    ▼
DriverComm::ReceiveRequest(fd)
    │ reads frame from specific fd
    ▼
InputMediator
    │ parses command type (READ/WRITE/FLUSH)
    ▼
CommandFactory
    │ creates ICommand object
    ▼
ThreadPool / Work Priority Queue
    │ enqueues: WRITE > READ > FLUSH (priority)
    │
    ▼
ICommand::Execute()
    │ calls RAIDStorage::Read/Write/Flush
    ▼
RAIDStorage (IStorage)
    │ RAID logic:
    ├─ Stripe offset → primary minion + mirror minion
    ├─ Call IStorageMinion::Read/Write
    ├─ Call IMetadataStore::Save on write
    │
    └─ Return result
    
    ▼
DriverComm::SendReply(data)
    │ writes response frame to fd
    ▼
Client Response
```

**Key properties:**
- **No blocking:** Reactor never waits; one thread processes all I/O events
- **Per-minion concurrency:** Each IStorageMinion has independent RWLock (N-way parallelism)
- **Decoupled execution:** ThreadPool prioritization is invisible to storage logic
- **Interface-driven:** RAIDStorage never knows if minions are memory/file/network

### Dependency Injection

**All dependencies are created during initialization and injected through abstract interfaces. RAIDStorage never constructs its own dependencies.**

```
Application Startup
    │
    ├─ StorageFactory::CreateMinions(backend, path, count, size)
    │   └─ Returns std::vector<std::unique_ptr<IStorageMinion>>
    │
    ├─ MetadataFactory::Create(backend, path)
    │   └─ Returns std::unique_ptr<IMetadataStore>
    │
    └─ RAIDStorage(minions, metadata_store)
        └─ No knowledge of concrete implementations
```

**Benefits:**
- RAIDStorage focuses on RAID algorithms, not infrastructure details
- Backends can change (memory → file → network) without modifying RAIDStorage
- Unit tests can inject mock implementations
- Configuration happens at startup time, not compiled in

---

### Two Modes

| Mode | Interface | Use case |
|---|---|---|
| **NBD** | Linux kernel block device `/dev/nbd0` | Local Linux block storage with RAID01 minions |
| **TCP** | Multi-client binary protocol over TCP | Remote clients (Mac ↔ Linux) with reconnection support |

Both modes use the same `InputMediator` → `RAIDStorage` pipeline. `TCPDriverComm` and `NBDDriverComm` both implement `IDriverComm`.

### TCP Features (Phase 2A+)
- **Multi-client**: Reactor handles multiple concurrent TCP clients on same port
- **Client reconnection**: Supports resuming sessions after disconnect
- **LIST_OFFSETS**: Auto-discover all stored offsets (useful for client state recovery)
- **Persistent allocation metadata**: Offset→size index in `.tcp_allocations` (TCP metadata file)
- **Pluggable storage backends**: `--backend memory` (fast, volatile) or `--backend file` (persistent disk)
- **FILE backend persistence**: Raw blocks in `minion*.img`, logical metadata in `raid_metadata.dat`
- **RAID01-inspired layout**: Striped across minions with mirroring for durability

---

## Core Components

### Storage (RAID + Abstraction)

| Component | Description |
|---|---|
| **RAIDStorage** | RAID01-inspired striping, mirroring, failover — pure RAID logic (no backend knowledge) |
| **StorageFactory** | Creates ready-made minions and metadata stores at initialization |
| **IStorageMinion** | Abstract minion backend interface with per-minion RWLock |
| **StorageMinionMemory** | In-memory backend (`std::vector<char>`) — fast, volatile |
| **StorageMinionFile** | Disk-backed backend (`pread`/`pwrite` to `.img` files) — persistent |
| **IMetadataStore** | Abstract metadata persistence interface |
| **MemoryMetadataStore** | Volatile metadata (lost on restart) |
| **FileMetadataStore** | Persistent metadata (`raid_metadata.dat`) |

### Transport (NBD + TCP)

| Component | Description |
|---|---|
| **NBDDriverComm** | Linux NBD kernel interface (IDriverComm) |
| **TCPDriverComm** | Multi-client TCP server (Reactor-based, reconnection support) |
| **InputMediator** | Routes commands (READ/WRITE/FLUSH/TRIM/GET_SIZE/LIST_OFFSETS) to storage |

### Concurrency & Execution

| Component | Description |
|---|---|
| **Reactor** | `epoll`-based event loop for non-blocking I/O multiplexing |
| **CommandFactory** | Creates ICommand objects (READ/WRITE/FLUSH) at runtime |
| **ThreadPool + WPQ** | Priority work queue (WRITE > READ > FLUSH) with fixed thread pool |
| **ICommand** | Encapsulates work items with priority for queue ordering |

### Infrastructure & Patterns

| Component | Description |
|---|---|
| **Dispatcher & Observer** | Type-safe event routing with minimal overhead |
| **Plugin System** | DirMonitor (inotify) + dlopen for runtime feature loading |
| **Logger** | Thread-safe singleton with DEBUG/INFO/ERROR levels |
| **Design Patterns** | Reactor, Factory, Command, Observer, Strategy, Plugin |

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

### TCP Mode (Multi-Client, Reconnection-Safe)

```bash
# Terminal 1 — start the server with 4 minions (in-memory, volatile):
./lds_app tcp 9999 1048576 4 memory

# Or with file-backed storage (persistent disk):
./lds_app tcp 9999 1048576 4 file ./minion_data

# Terminal 2 — run the Python client:
python3 client.py

# Or connect from a remote machine (e.g. Mac via Tailscale):
python3 client.py --host 100.x.x.x

# Multiple clients can connect simultaneously — each gets independent fd
```

**Features (Memory Backend):**
- Data persists across client reconnections while the server is running
- Auto-discover all offsets on reconnect via `LIST_OFFSETS`
- Append-only writes to same offset accumulate
- Server logs RAID stripe distribution: `[RAID-WRITE] offset=X → primary[Y] + mirror[Z]`

**Features (File Backend):**
- All of the above, PLUS:
- Raw block data persists in `minion*.img` files (non-volatile storage)
- Logical metadata persists in `raid_metadata.dat` (offset→size map)
- Server restart recovers all previously-written data and allocation metadata
- `raid_metadata.dat` is binary format: pairs of (size_t offset, size_t size)

### TCP Mode — Minion Count & Backend

```bash
# Use hardware_concurrency minions with memory backend (default):
./lds_app tcp 9999 1048576

# Use specific minion count (must be even: N primary + N mirror):
./lds_app tcp 9999 1048576 2 memory      # 2 minions: 1 primary + 1 mirror
./lds_app tcp 9999 1048576 4 memory      # 4 minions: 2 primary + 2 mirror (recommended)
./lds_app tcp 9999 1048576 8 memory      # 8 minions: 4 primary + 4 mirror

# Use file backend (requires backend-path):
./lds_app tcp 9999 1048576 4 file ./data         # Creates minion0.img, minion1.img, etc.
./lds_app tcp 9999 1048576 4 file /mnt/storage   # Persistent storage on mounted volume
```

### NBD Mode (Linux only, requires root, RAID01 striping)

```bash
# Load the NBD kernel module:
sudo modprobe nbd max_part=8

# Start the server with RAID minions (memory backend):
sudo ./lds_app nbd /dev/nbd0 1048576 4 memory

# Or with persistent file backend:
sudo ./lds_app nbd /dev/nbd0 1048576 4 file ./minion_data

# Mount as a filesystem (in another terminal):
sudo mkfs.ext4 /dev/nbd0
sudo mount /dev/nbd0 /mnt/lds
```

---

## TCP Wire Protocol

Requests and responses are big-endian binary frames:

```
Request (24 bytes header + optional payload):
  [type   : uint32 BE]   0=READ 1=WRITE 2=DISCONNECT 3=FLUSH 4=TRIM 5=GET_SIZE 6=LIST_OFFSETS
  [handle : uint64 BE]   unique request ID for matching responses
  [offset : uint64 BE]   byte offset / context identifier
  [length : uint32 BE]   payload length in bytes
  [data   : length bytes] payload

Response (16 bytes header + optional payload):
  [error  : uint32 BE]   0 = success, EIO on failure
  [handle : uint64 BE]   matches request handle
  [length : uint32 BE]   response payload length or metadata value
  [data   : length bytes] payload (READ=data, LIST_OFFSETS=offset pairs)

Operations:
  READ:       Retrieve data from offset
  WRITE:      Store data at offset
  GET_SIZE:   Query size of data at offset (returned in header.length)
  LIST_OFFSETS: Enumerate all stored offsets (returned as [offset(8) size(8)]* payload)
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

## Key Architectural Decisions

### Layered Storage Abstraction

**Storage Factory Pattern** — RAIDStorage doesn't know about backend types (Memory/File/Network/RDMA/UCX). StorageFactory owns backend selection and instantiation. RAIDStorage receives ready-made minions through `IStorageMinion` interface.

**Metadata Store Abstraction** — RAIDStorage doesn't know how to persist metadata (File/Memory/SQL/Redis). IMetadataStore owns persistence strategy. The `Save(map)` API means RAIDStorage owns logical state; the store just persists it.

**Data ≠ Metadata Separation** — Raw blocks live in minions (via IStorageMinion), offset→size mappings live in metadata stores (via IMetadataStore). Each has independent persistence strategy and can evolve separately.

### Concurrency & Load Distribution

**Per-Minion RWLock** — Each minion has independent `shared_mutex` for concurrent reads + exclusive writes. N minions = N-way parallelism with no global bottleneck.

**RAID01-Inspired Striping** — Distributes offsets across minions, replicates to mirrors. Reads from primary (fast path), writes to primary + mirror for durability.

**epoll + Reactor** — Single thread handles all I/O events without blocking. Reactor multiplexes all client fds via `epoll_wait`, dispatching events to handlers. Eliminates thread-per-connection overhead.

### Transport Independence

**IDriverComm Interface** — NBD and TCP are interchangeable implementations. Swapping transports requires only `LDS.cpp:main()` — zero changes to InputMediator or RAIDStorage.

**LIST_OFFSETS Protocol** — Enables clients to reconnect and discover all stored offsets. Supports stateless clients that resume via server-side state enumeration.

### Execution Abstraction

**CommandFactory + ThreadPool** — Work is represented as ICommand objects. CommandFactory creates them; ThreadPool prioritizes (WRITE > READ > FLUSH) without blocking I/O.

**Templates for Observer/Factory** — Avoids virtual dispatch overhead in hot path. `Dispatcher<T>` instantiated at compile time per event type.

---

## Technologies

C++20 · Linux · epoll · inotify · NBD (Network Block Device) · TCP/UDP sockets · pthreads · dlopen/dlsym · POSIX · GNU Make · Docker · Python 3

---

## Performance Characteristics

| Aspect | Property |
|---|---|
| **I/O Multiplexing** | `epoll`-based; Reactor does not execute storage work, dispatches ready fds and returns to epoll |
| **Concurrency Model** | per-minion RWLock, no global storage lock — N minions handle 1/N offset space independently |
| **Request Execution** | Asynchronous via ThreadPool with priority queue (WRITE > READ > FLUSH) |
| **Data Path** | Single-threaded Reactor detects fd readiness; InputMediator reads request, enqueues command, returns to epoll |
| **Memory Overhead** | Metadata size (offset→size mappings) is O(write_count); Memory backend allocates vectors; File backend stores data in image files |
| **Backend Independence** | Storage algorithms never know minion type — Memory vs File performance characteristics are encapsulated |

---

## Known Limitations

These are intentional constraints or areas for future improvement:

| Limitation | Impact | Future Direction |
|---|---|---|
| **Synchronous Mirroring** | Writes wait for both primary + mirror to complete before responding to client | Async mirror writes with eventual consistency option |
| **Metadata Persistence** | Metadata saved after each write (fsync) — limits write throughput on FILE backend | Batched metadata updates, separate WAL (write-ahead log) |
| **Single RAID Controller** | Only one server process manages RAID logic — no distributed control | Consensus-based RAID controller with failover |
| **No Automatic Mirror Rebuild** | If primary fails, mirror is read-only until manual intervention | Automatic mirror promotion and rebuild from backup |
| **No Checksum Validation** | Data integrity depends on filesystem; no end-to-end checksums | CRC32/SHA256 per-block validation |
| **Transient Metadata Loss (Memory Backend)** | Server restart loses all allocation metadata | Configurable metadata durability guarantees |
| **Single Reactor Thread** | One thread multiplexes all fds; may become bottleneck under very high event rates | Work-stealing or io_uring-based multiplexing on future kernels |

---

## Implementation Details

### RAID01-Inspired Striping

The storage layout distributes load across minions and replicates for durability:

```
4 Minions Example (2 primary + 2 mirror):

Offset 0 → stripe_idx=0 → primary[0] + mirror[2]
Offset 1 → stripe_idx=0 → primary[0] + mirror[2]
Offset 2 → stripe_idx=1 → primary[1] + mirror[3]
Offset 3 → stripe_idx=1 → primary[1] + mirror[3]
...

Reads:  primary[idx], fallback to mirror[idx] on error
Writes: primary[idx] + mirror[idx]
        Each minion has std::shared_mutex — concurrent reads, exclusive writes
```

**Properties:**
- N minions handle 1/N of offset space (no single bottleneck)
- Per-minion RWLock enables N-way parallelism
- All writes replicated to mirror minion for durability
- Read from mirror if primary fails

### StorageMinionMemory Implementation

Uses `std::vector<char>` for fast in-memory storage. Suitable for testing and transient workloads.

### StorageMinionFile Implementation

Uses `pread`/`pwrite` system calls for positional file I/O to `minion*.img` files. Pre-allocates space with `ftruncate()` on creation.

### FileMetadataStore Format

Binary format for `raid_metadata.dat`:
```
[offset_0 (8 bytes)][size_0 (8 bytes)]
[offset_1 (8 bytes)][size_1 (8 bytes)]
...
```

Loaded entirely into memory on startup; persisted after every write operation.

### Reactor Event Loop

Uses Linux `epoll` for efficient I/O multiplexing. Single thread processes all events without blocking.

### ThreadPool Priority Queue

WRITE commands execute first (metadata consistency), then READ, then FLUSH. Priority ensures writes don't starve but reads don't block writes.

---

## Roadmap

### Completed

- [x] **Phase 1** — Core framework (Reactor, NBD, plugin system, mediator)
- [x] **Phase 2A** — Multi-client TCP bridge with RAID01-inspired minions
- [x] **Phase 2B** — Pluggable storage backends with factory pattern

### Planned

- [ ] **Phase 2** — Network-based minion transport
  - [ ] RemoteStorageMinion abstraction for network backends
  - [ ] TCP-based remote minion protocol
  - [ ] RDMA support (if demand exists)



### Exploratory (Research, not committed)

- Optional: Raspberry Pi lightweight minion variant
- Optional: End-to-end checksums (CRC32/SHA256 per-block)
- Optional: Async mirror writes with eventual consistency option
- Optional: Write-ahead log for metadata batching
- Optional: Automatic mirror rebuild from backup nodes
