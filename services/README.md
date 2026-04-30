# Services (`/services`)

## Purpose

This directory is reserved for **microservice implementations** that will be built on top of the Local Cloud infrastructure. Services are plugins that:
- Register themselves with the service registry
- Discover and communicate with other services
- Implement standardized service interfaces
- Support health checking and monitoring

## Current Status

🔄 **Phase 2 (Service Registry & Discovery) - Design Phase**

This directory is prepared for Phase 2 implementation and is currently in planning stages.

## Directory Structure

```
services/
├─ README.md                          # This file
└─ communication_protocols/           # Protocol definitions
   └─ (protocols to be designed)
```

## What Will Be Here

### Example Services (Phase 2+)

#### DatabaseService
```cpp
class DatabaseService : public IService {
    // Handle database operations
    // Registered as "database" in service registry
};
```

#### APIService
```cpp
class APIService : public IService {
    // Handle HTTP requests
    // Discovers DatabaseService from registry
    // Communicates via message broker
};
```

#### CacheService
```cpp
class CacheService : public IService {
    // Provide caching layer
    // Service-to-service caching
};
```

## Service Architecture

### Service Lifecycle (Phase 2)
```
1. Plugin loaded via LDS (Phase 1) ✅
2. Service implements IService interface
3. Service registers with ServiceRegistry
4. Other services discover it by name
5. Communication via MessageBroker (Phase 3)
6. Health checks monitored continuously
7. Auto-scaling via ResourceScheduler (Phase 4)
```

### Service Communication Flow (Phase 3)
```
Service A                 Service B
    │                        │
    ├─ Discover Service B ───┤
    │                        │
    ├─ Send RPC Request ─────┤
    │                        │
    ├─ Wait for Response ◄───┤
    │                        │
```

## Related Components

### Phase 1: Dynamic Plugin Loading ✅
- **Location**: [`/plugins`](../plugins/README.md)
- **Status**: Complete
- Provides the foundation for loading services

### Phase 2: Service Registry (Current Planning)
- **Components to build**:
  - ServiceRegistry (singleton)
  - IService interface
  - Service metadata
  - Health check system

### Phase 3: RPC & Message Passing (Planned)
- **Components to build**:
  - MessageBroker
  - Request/response protocol
  - Async message handling

### Phase 4: Resource Scheduling (Planned)
- **Components to build**:
  - ResourceScheduler
  - LoadBalancer
  - Service quotas

## Service Registration Flow

When Phase 2 is complete, services will follow this pattern:

```cpp
// Service Plugin Implementation
class DatabaseService : public IService {
public:
    void onServiceLoad() override {
        // Register with ServiceRegistry
        ServiceRegistry::getInstance()
            ->registerService("database", this);
        
        // Start service operations
        startListening();
    }
    
    void onServiceUnload() override {
        // Cleanup and deregister
        ServiceRegistry::getInstance()
            ->unregisterService("database");
    }
};

// Auto-registration on plugin load
__attribute__((constructor)) void init() {
    new DatabaseService();
}
```

## Service Discovery (Phase 2)

```cpp
// How another service discovers and uses a service
class APIService : public IService {
    void onServiceLoad() override {
        // Discover database service
        IService* db = ServiceRegistry::getInstance()
            ->discoverService("database");
        
        if (db) {
            std::cout << "✅ Database service found!" << std::endl;
            // Will communicate via MessageBroker in Phase 3
        }
    }
};
```

## Protocol Definitions (Phase 2-3)

### Service Interface
```cpp
class IService {
public:
    virtual ~IService() = default;
    virtual std::string getName() = 0;
    virtual std::string getVersion() = 0;
    virtual ServiceStatus healthCheck() = 0;
    virtual void shutdown() = 0;
};
```

### RPC Message Format (Phase 3)
```json
{
    "id": "msg-123",
    "from": "api-service",
    "to": "database-service",
    "method": "query",
    "params": {
        "sql": "SELECT * FROM users"
    },
    "timestamp": 1234567890
}
```

## Communication Protocols

### Inter-Service Communication Layers

```
Layer 4: Application Logic (Services)
            │
            ├─ Service: Database
            ├─ Service: API
            └─ Service: Cache
            │
Layer 3: RPC Framework (Phase 3)
            │
            ├─ MessageBroker
            ├─ Request/Response
            └─ Topic-based Pub/Sub
            │
Layer 2: Network Transport
            │
            ├─ TCP/IP
            ├─ Message Serialization
            └─ Encryption (Phase 6)
            │
Layer 1: Plugin System (Phase 1) ✅
            │
            ├─ Dynamic Loading
            ├─ Process Memory
            └─ RAII Resource Management
```

## Deployment Model

### Current (Phase 1)
```
Single Process
├─ Main Application (LDS)
└─ Multiple Plugins (.so files)
    ├─ Plugin 1
    ├─ Plugin 2
    └─ Plugin N
```

### Phase 2+
```
Single Process / Multiple Services
├─ Main Container (LDS)
└─ Service Plugins
    ├─ Database Service (discovery, health)
    ├─ API Service (RPC communication)
    ├─ Cache Service (resource quotas)
    └─ ... More Services
```

### Phase 2+ (Future: Distributed)
```
Multiple Machines
├─ Edge Node 1
│  └─ Services {A, B, C}
│
├─ Edge Node 2
│  └─ Services {B, D, E}
│
└─ Central Hub
   └─ Registry & Load Balancer
```

## Service Features Timeline

| Feature | Phase | Status |
|---------|-------|--------|
| Plugin loading | 1 | ✅ Done |
| Service registration | 2 | 🔄 Planned |
| Service discovery | 2 | 🔄 Planned |
| Health checking | 2 | 🔄 Planned |
| RPC communication | 3 | 📋 Planned |
| Async messaging | 3 | 📋 Planned |
| Load balancing | 4 | 📋 Planned |
| Resource quotas | 4 | 📋 Planned |
| Service authentication | 6 | 📋 Planned |
| Distributed deployment | 5+ | 📋 Future |

## Next Steps

### Phase 2 Implementation Plan

1. **Design ServiceRegistry**
   - Service metadata structure
   - Discovery API
   - Health check mechanism

2. **Create IService Interface**
   - Base class for all services
   - Lifecycle hooks
   - Metadata provider

3. **Implement Service Manager**
   - Load services from plugins
   - Handle service lifecycle
   - Monitor health

4. **Add Tests**
   - Service registration tests
   - Discovery tests
   - Multi-service scenarios

---

**Phase**: 2 (Service Registry & Discovery) 🔄  
**Status**: Design phase - ready for implementation  
**Depends On**: Phase 1 (Dynamic Plugin Loading) ✅
