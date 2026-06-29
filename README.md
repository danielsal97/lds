# LDS — Local Drive Storage

A modular RAID-based storage architecture built in **C++20** on Linux, designed for future distributed storage nodes.  
From the user's perspective it is a regular block device. Under the hood, data is stored across independent storage minions with RAID01-inspired striping and mirroring.

---

## Architecture

```
User process
    │
    ▼
┌─────────────────────────────────────────────────┐
│  DriverComm (IDriverComm interface)             │
│  ┌─────────────┐        ┌──────────────┐       │
│  │ NBDDriverComm       │ TCPDriverComm │       │
│  │ (kernel I/O)       │ (multi-client) │       │
│  └─────────────┘        └──────────────┘       │
└────────────┬──────────────────────────────────┘
             │
             ▼
┌─────────────────────────────────────────────────┐
│  InputMediator                                  │
│  (read/write/flush/trim dispatcher)             │
└────────────┬──────────────────────────────────┘
             │
             ▼
┌─────────────────────────────────────────────────┐
│  IStorage (abstract storage interface)          │
│  ┌──────────────┐           ┌────────────────┐ │
│  │ LocalStorage │           │  RAIDStorage   │ │
│  │ (deprecated) │           │  (striping +   │ │
│  │              │           │   mirroring)   │ │
│  └──────────────┘           └────────┬────────┘ │
└─────────────────────────────────┬─────────────┘
                                  │
                    ┌─────────────┼─────────────┐
                    ▼             ▼             ▼
              ┌──────────────────────────────────┐
              │    IStorageMinion (abstract)     │
              │   with per-minion RWLock         │
              └──────┬────────────────┬──────────┘
                     │                │
         ┌───────────┘                └────────────┐
         ▼                                         ▼
  ┌────────────────┐                      ┌─────────────────┐
  │StorageMinionMem│                      │StorageMinionFile│
  │(in-memory      │                      │(disk-backed .img)
  │std::vector)    │                      │pread/pwrite I/O)│
  └────────────────┘                      └────────┬────────┘
         │                                         │
   ┌─────┴──────┬──────────┐              ┌────────┴─────────┐
   ▼            ▼          ▼              ▼                  ▼
Minion 0    Minion 1   Minion N      raid_metadata.dat  minion*.img
(memory)    (memory)   (memory)      (offset→size map)  (raw blocks)
```

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

## Components Built

### Phase 1 — Core Framework

| Component | Path | Description |
|---|---|---|
| **Reactor** | `design_patterns/reactor/` | `epoll`-based event loop — multi-client I/O event dispatcher |
| **InputMediator** | `services/mediator/` | Routes READ/WRITE/FLUSH/TRIM/GET_SIZE/LIST_OFFSETS to storage |
| **RAIDStorage** | `services/local_storage/` | RAID01-inspired striping with per-minion locking; persists metadata to `raid_metadata.dat` |
| **IStorageMinion** | `services/local_storage/` | Abstract pluggable backend interface with per-minion RWLock |
| **StorageMinionMemory** | `services/local_storage/` | In-memory backend using `std::vector<char>` — fast, volatile |
| **StorageMinionFile** | `services/local_storage/` | Disk-backed backend using pread/pwrite to `minion*.img` files — persistent |
| **NBDDriverComm** | `services/communication_protocols/nbd/` | Reads `nbd_request` from kernel, writes `nbd_reply` back |
| **Factory** | `design_patterns/factory/` | Creates commands and plugins by name at runtime |
| **Observer** | `design_patterns/observer/` | `Dispatcher<T>` + `CallBack<T,Sub>` — type-safe event routing |
| **Command** | `design_patterns/command/` | `ICommand` interface with priority (WRITE > READ > FLUSH) |
| **ThreadPool + WPQ** | `utilities/threading/` | Priority work queue + fixed thread pool for async execution |
| **Plugin System** | `plugins/` | `DirMonitor` (inotify) + `PNP` + `soLoader` — loads `.so` plugins at runtime via `dlopen` |
| **Logger** | `utilities/logger/` | Thread-safe singleton logger with DEBUG/INFO/ERROR levels |

### Phase 2A — Multi-Client TCP Bridge

| Component | Path | Description |
|---|---|---|
| **TCPDriverComm** | `services/communication_protocols/tcp/` | Multi-client TCP server (Reactor-based) with reconnection support |
| **TCP Persistent Metadata** | `.tcp_allocations` | Binary file tracking TCP client session allocations |
| **RAID Persistent Metadata** | `raid_metadata.dat` | Binary file tracking RAID logical offset→size map (FILE backend only) |
| **LIST_OFFSETS** | `InputMediator` | Query all stored offsets (for client auto-discovery on reconnect) |
| **Interfaces** | `interfaces/` | Shared `IDriverComm`, `IMediator`, `IStorage` headers |
| **TCP client** | `test_client.py` | Python client for testing TCP protocol |

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

## RAID01-Inspired Striping

**Load Distribution + Mirroring (In-Memory Layout):**

Current implementation uses in-memory minions. The storage layout is inspired by RAID01 principles:

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

**Architectural Strengths:**
- **Load distribution**: N minions handle 1/N of offset space (no single bottleneck)
- **Per-minion concurrency**: Each StorageMinion has independent RWLock (N-way parallelism)
- **Durability**: All writes replicated to mirror minion
- **Failover**: Read from mirror if primary fails
- **Backend abstraction**: StorageMinion interface allows swapping memory → Raspberry Pi → NBD → network storage

---

## Key Technical Decisions

**Why epoll + Reactor?** Single thread handles all I/O events (no thread-per-connection overhead). Reactor dispatches each fd event to handler and returns to `epoll_wait` immediately. Multi-client TCP via per-client fd tracking.

**Why per-minion RWLock instead of global mutex?** Each `StorageMinion` has `shared_mutex` for concurrent reads + exclusive writes. N minions = N-way parallelism with no global bottleneck. RAID stripe distributes offsets across minions.

**Why RAID01-inspired striping?** Distributes load across minions and replicates to mirror. No single bottleneck. Reads from primary (fast path), writes sync to primary + async to mirror for durability.

**Why IStorageMinion interface?** Enables pluggable storage backends without changing RAID logic or communication layers. Current: StorageMinionMemory (fast, volatile) and StorageMinionFile (persistent). Future: Raspberry Pi, NBD, network storage. Each backend handles its own durability strategy.

**Why separate StorageMinionMemory and StorageMinionFile?** Clean separation of concerns. StorageMinionMemory uses `std::vector<char>` for speed. StorageMinionFile uses pread/pwrite to `.img` files for persistence. RAIDStorage doesn't know or care—it only calls the IStorageMinion interface.

**Why raid_metadata.dat (separate from minion*.img)?** RAIDStorage persists the logical offset→size map independently of raw block storage. StorageMinionFile persists data; RAIDStorage persists metadata. On restart: StorageMinionFile recovers raw blocks, LoadMetadata() recovers the mapping. This separation enables future backends (e.g., remote minions, cloud storage) to have different persistence strategies.

**Why `TCPDriverComm` implements `IDriverComm`?** Complete transport abstraction. Swapping NBD ↔ TCP requires only `LDS.cpp:main()` — zero changes to InputMediator, RAIDStorage, or StorageMinion.

**Why LIST_OFFSETS operation?** Clients can reconnect and discover all previously-stored offsets. Enables stateless clients that resume via server-side state enumeration.

**Why persistent allocation metadata?** `.tcp_allocations` stores TCP session allocations; `raid_metadata.dat` stores RAID logical allocations. FILE backend survives server restart with full state recovery. Supports complex client workflows: write → disconnect → restart server → reconnect → LIST_OFFSETS → resume.

**Why templates for Observer/Factory?** Avoids virtual dispatch overhead in hot path. `Dispatcher<T>` instantiated at compile time per event type.

---

## Technologies

C++20 · Linux · epoll · inotify · NBD (Network Block Device) · TCP/UDP sockets · pthreads · dlopen/dlsym · POSIX · GNU Make · Docker · Python 3

---

## Storage Abstraction

**The critical architectural insight: complete separation of interface from implementation at multiple layers.**

```
IStorage (abstract storage interface)
    ▲
    │
    ├──────────────┬──────────────┐
    │              │              │
LocalStorage   RAIDStorage    (Future)
(deprecated)   (current)      (UDP, etc.)
                   │
                   │ persists metadata to →
                   │
            raid_metadata.dat
            (offset→size map)
                   │
                   ├─────────────────────────────────┐
                   │                                 │
        IStorageMinion (abstract interface)          │
        (per-minion pluggable backend)               │
            ▲                                        │
            │                                        │
    ┌───────┼───────────┐                            │
    │       │           │                            │
  Memory   File       (Future)                       │
  backend  backend    Network/NBD/etc.               │
    │       │           │                            │
    ├───────┴───────────┤                            │
    │                   │                            │
 volatile          minion*.img files ◄──────────────┘
 (data loss         (persistent
  on restart)        raw blocks)
```

**Multi-layer abstraction:**
1. **IStorage** — Decouples DriverComm (NBD/TCP) from storage strategy
2. **IStorageMinion** — Decouples RAIDStorage from minion backend (memory/file/network)
3. **MinionBackend enum** — Runtime selection of backend (not compile-time)

The InputMediator, Reactor, and DriverComm layers **never directly depend on storage implementation**. They only know about `IStorage` interface. RAIDStorage knows only about `IStorageMinion` interface, not specific backends. This means:

- New storage backends (StorageMinionFile, StorageMinionNetwork, StorageMinionNBD) can be added without modifying RAIDStorage, InputMediator, or drivers
- Persistence strategy can be chosen at runtime (CLI flag): `--backend memory` vs `--backend file`
- Unit tests can mock `IStorage` or `IStorageMinion` at any layer
- Data reliability evolves without rewriting: memory (fast) → file (durable) → network (distributed)

This is not just an OOP exercise — it's a fundamental architectural principle that keeps concerns separated and enables the system to evolve from single-node (memory) → persistent (disk) → distributed (network minions) without rewriting the core logic.

---

## Roadmap

- [x] **Phase 1** — Core framework (Reactor, NBD, plugin system, mediator)
- [x] **Phase 2A** — Multi-client TCP bridge with RAID01-inspired minions
  - [x] Reactor-based multi-client support
  - [x] Client reconnection
  - [x] LIST_OFFSETS for offset enumeration
  - [x] Persistent allocation metadata (`.tcp_allocations`)
  - [x] RAID01-inspired layout across minions
  - [x] Per-minion RWLock (N-way parallelism, no bottleneck)
- [x] **Phase 2B** — Pluggable storage backends (partial)
  - [x] IStorageMinion abstract interface
  - [x] StorageMinionMemory (in-memory backend)
  - [x] StorageMinionFile (disk-backed .img files with pread/pwrite)
  - [x] Runtime backend selection via CLI (--backend memory|file)
  - [x] Metadata persistence (raid_metadata.dat for FILE backend)
  - [x] Swap backends without changing RAID/mediator logic
  - [ ] Factory pattern for cleaner backend instantiation (future refactor)
- [ ] **Phase 2C** — Additional backends
  - [ ] StorageMinionNBD (Linux NBD over network)
  - [ ] StorageMinionTCP (custom TCP-based remote minion)
- [ ] **Phase 3** — Distributed nodes
  - [ ] Minion daemon (standalone server exposing StorageMinion interface)
  - [ ] Remote minion discovery and health checks
  - [ ] Failover (read from mirror if primary unreachable)
- [ ] **Phase 4** — Heterogeneous cluster
  - [ ] Lightweight minion for Raspberry Pi
  - [ ] Unified cluster API (any storage backend in same RAID group)
  - [ ] Automatic replication to backup nodes
