# NBD (Network Block Device) Communication Protocol

## Overview

This module implements the **Network Block Device (NBD)** protocol, which allows the LDS storage system to expose a virtual block device to the Linux kernel. The kernel communicates with this module via standard read/write/trim/flush operations, treating the storage system as a regular block device from the OS perspective.

## Purpose

The NBD protocol bridges the gap between:
- **User-space storage system**: The LDS service running in userland with custom storage logic
- **Kernel block device interface**: The Linux kernel's standard block device API

This abstraction enables flexible storage backends (local storage, network storage, etc.) to be exposed as if they were standard block devices.

## Components

### Headers

#### `IDriverComm.hpp`
Abstract interface for driver communication protocols. This allows the system to swap different protocols (NBD, iSCSI, etc.) without changing the storage layer logic.

**Key Methods:**
- `ReceiveRequest()` - Get incoming requests from the kernel
- `SendReplay()` - Send responses back to the kernel
- `Disconnect()` - Clean up connections
- `GetFD()` - Get file descriptor for epoll/event monitoring

#### `NBDDriverComm.hpp`
Concrete implementation of the NBD protocol. Handles:
- Socket communication with the NBD kernel driver
- NBD request/reply message encoding and decoding
- Lifecycle management (handshake, signal handling, cleanup)

**Key Features:**
- Two constructors: one for total storage size, one for block-level specification
- Background listener thread for handling kernel communications
- Signal handling for graceful shutdown

#### `DriverData.hpp`
Standardized data container for all requests flowing between the driver and storage system.

**Action Types:**
- `READ` - Read data from storage
- `WRITE` - Write data to storage
- `FLUSH` - Sync data to persistent storage
- `TRIM` - Deallocate/trim blocks
- `DISCONNECT` - Close connection

**Status Types:**
- `SUCCESS` - Operation completed successfully
- `FAILURE` - Operation failed

### Implementation Files

- `NBDDriverComm.cpp` - NBD protocol implementation
  - Socket setup with `socketpair()`
  - NBD request/reply message parsing using `linux/nbd.h` definitions
  - Thread-safe communication with the kernel driver
  - Proper byte-order conversion (network byte order handling)

- `DriverData.cpp` - DriverData implementation

## Architecture

### Communication Flow

```
Kernel (block device layer)
         ↓
    NBD Protocol
         ↓
NBDDriverComm::ReceiveRequest() → DriverData
     (parse NBD messages)
         ↓
    Storage System
         ↓
DriverData → NBDDriverComm::SendReplay()
  (encode NBD response)
         ↓
    Kernel
```

### Threading Model

- **Main thread**: Calls `ReceiveRequest()` and `SendReplay()` for each kernel request
- **Listener thread**: Background thread handling low-level socket operations
- **Signal thread**: Handles graceful shutdown signals (SIGINT, SIGTERM)

## Usage Example

```cpp
#include "NBDDriverComm.hpp"
#include "DriverData.hpp"

// Create NBD device with 1GB capacity
NBDDriverComm nbd("/dev/nbd0", 1024 * 1024 * 1024);

// Process kernel requests
while (true) {
    auto request = nbd.ReceiveRequest();
    
    if (request->m_type == DriverData::READ) {
        // Handle read operation
        request->m_status = DriverData::SUCCESS;
    } else if (request->m_type == DriverData::WRITE) {
        // Handle write operation
        request->m_status = DriverData::SUCCESS;
    }
    
    nbd.SendReplay(request);
}

nbd.Disconnect();
```

## Error Handling

- **DriverError**: Base exception for driver-related errors
- **NBDDriverError**: NBD-specific errors (socket failures, protocol errors)
- All I/O operations throw exceptions on failure

## Key Implementation Details

1. **Socket Communication**: Uses `socketpair()` to create a bidirectional communication channel with the NBD kernel driver
2. **Message Format**: Adheres to the Linux NBD protocol specification (`linux/nbd.h`)
3. **Byte Order**: Proper network byte order conversion using `htonl()`, `ntohl()`, `be64toh()`
4. **Robustness**: Handles partial reads/writes with retry logic (`ReadAll`, `WriteAll`)

## Dependencies

- Linux kernel NBD driver (`/sys/module/nbd/`)
- Standard C++ libraries (thread, memory)
- Linux system headers (`linux/nbd.h`, `sys/socket.h`, `sys/ioctl.h`)

## Integration Points

This module is used by the LDS (Local Data Storage) service to:
1. Expose storage to the kernel as a block device
2. Receive and process block device operations
3. Return results to the kernel in the NBD protocol format

Other protocol implementations can follow the same `IDriverComm` interface for different communication protocols (e.g., iSCSI, ATA).
