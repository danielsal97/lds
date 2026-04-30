# Third-Party Dependencies (`/third_party`)

## Purpose

This directory contains **external libraries and dependencies** used by the Local Cloud project. These are code written by other developers/organizations that our project depends on.

## Directory Structure

```
third_party/
├─ README.md                          # This file
└─ (external library directories)
   ├─ library1/
   │  ├─ include/
   │  ├─ src/
   │  └─ LICENSE
   ├─ library2/
   └─ ...
```

## Common Third-Party Libraries

### System Libraries (Pre-installed)
- `glibc` - C standard library
- `pthreads` - POSIX thread API
- `linux` - Linux kernel headers

### Optional Dependencies (May be in third_party)

| Library | Purpose | Usage |
|---------|---------|-------|
| inotify | Filesystem monitoring | DirMonitor, filesystem events |
| boost | C++ utilities | (if used, typically not) |
| json | JSON parsing | Configuration, RPC payloads (Phase 3) |
| openssl | Encryption | Security layer (Phase 6) |
| sqlite | Database | Local persistence (Phase 5) |

## Including Third-Party Code

### From System
```cpp
#include <pthread.h>        // System library
#include <dirent.h>         // POSIX directory API
#include <sys/inotify.h>    // Linux inotify
```

### From Third-Party
```cpp
#include "third_party/json/json.hpp"
#include "third_party/sqlite/sqlite3.h"
```

### In Makefile

```makefile
CFLAGS += -Ithird_party
LDFLAGS += -Lthird_party/lib

# Link with third-party library
LIBS += -ljson -lsqlite3
```

## License Management

⚠️ **Important**: Verify license compatibility

### Checking Licenses

```bash
# View license of third-party component
cat third_party/library_name/LICENSE

# Common licenses
# MIT - Permissive, minimal restrictions
# Apache 2.0 - Permissive, requires license notice
# GPL - Copyleft, derivative works must be GPL
# LGPL - Weak copyleft, library can be used in proprietary code
```

### License Considerations for Local Cloud

| License | Allowed | Notes |
|---------|---------|-------|
| MIT | ✅ Yes | Use freely |
| Apache 2.0 | ✅ Yes | Include license notice |
| BSD | ✅ Yes | Permissive |
| GPL v3 | ⚠️ Careful | Entire project becomes GPL |
| LGPL | ✅ Yes | Dynamic linking okay |
| Proprietary | ❌ No | May violate terms |

### Including Third-Party in Project

```bash
# Copy license to source
cp third_party/json/LICENSE third_party/LICENSE_JSON

# Reference in main README
echo "Uses: JSON library (MIT License)" >> README.md
```

## Managing Dependencies

### Adding New Third-Party Library

1. **Download or Clone**
   ```bash
   cd third_party
   git clone https://github.com/author/library.git
   # or
   wget https://...library.tar.gz && tar -xz
   ```

2. **Verify License**
   ```bash
   cat third_party/library/LICENSE
   # Check compatibility
   ```

3. **Create Build Instructions**
   ```bash
   # third_party/library/BUILD.md
   # Compilation instructions
   ```

4. **Update Main Makefile**
   ```makefile
   # Add to project Makefile
   THIRD_PARTY_LIBS += -Lthird_party/library -lmylib
   ```

5. **Test Integration**
   ```bash
   make clean && make
   ./bin/test_integration
   ```

### Removing Dependencies

```bash
# 1. Remove from Makefile
# 2. Remove #includes from code
# 3. Remove directory
rm -rf third_party/old_library
# 4. Rebuild
make clean && make
```

## Version Management

### Tracking Versions

```bash
# third_party/versions.txt
# Library          | Version | License
# json            | 3.11.2  | MIT
# sqlite          | 3.40.0  | Public Domain
# openssl         | 3.0.0   | Apache 2.0
```

### Update Procedure

```bash
# 1. Backup old version
mv third_party/json third_party/json.bak

# 2. Download new version
wget https://github.com/nlohmann/json/releases/download/v3.12.0/json.hpp
cp json.hpp third_party/json/

# 3. Test
make clean && make
./bin/test_all

# 4. If successful, remove backup
rm -rf third_party/json.bak
```

## Common Issues

### Symbol Conflicts

```bash
# Problem: Symbol defined in multiple libraries
# Example: Two JSON libraries both define parse()

# Solution: Use namespace or rename
namespace json_v1 {
    #include "third_party/json_v1/json.hpp"
}
```

### Version Mismatch

```bash
# Problem: Code expects newer API than library provides
#include "third_party/old_json/json.hpp"
json.parse();  // ❌ Function not in old version

# Solution: Update library or adapt code
```

### Missing Dependencies

```bash
# Problem: Library depends on another library
# Error: undefined reference to 'function'

# Solution: Link with dependency
LDFLAGS += -ljson -lz  # json depends on zlib
```

## Build Strategy for Third-Party

### Option 1: Vendored (Included)
```
third_party/
├─ json/
├─ sqlite/
└─ openssl/

# Pros: Full control, reproducible builds
# Cons: Maintenance burden, license tracking
```

### Option 2: System Libraries
```bash
# Install system-wide
sudo apt-get install libjson-dev libsqlite3-dev

# Link to system libraries
LDFLAGS += -ljson -lsqlite3

# Pros: Smaller repo, system updates
# Cons: Dependency on system packages
```

### Option 3: Hybrid
```
# Core dependencies vendored
third_party/
├─ json/           # Vendored - simple, core
└─ sqlite/         # Vendored - complex, long-term

# System dependencies
# inotify          # System - standard Linux API
# pthreads         # System - standard POSIX
```

## Current Project Status

### Phase 1 Dependencies

| Dependency | Type | Location | Status |
|-----------|------|----------|--------|
| pthreads | System | `/usr/include` | ✅ Used |
| inotify | System | `/usr/include` | ✅ Used |
| libc | System | `/usr/lib` | ✅ Used |

### Phase 2+ Planned

| Dependency | Phase | Purpose | Status |
|-----------|-------|---------|--------|
| JSON | 3 | RPC serialization | 🔄 Planned |
| SQLite | 5 | Local storage | 🔄 Planned |
| OpenSSL | 6 | Encryption | 📋 Planned |

## Best Practices

### ✅ Do

- ✅ Keep third-party in separate directory
- ✅ Track versions explicitly
- ✅ Document licenses
- ✅ Pin versions for stability
- ✅ Test after updates
- ✅ Use stable releases (not main branch)

### ❌ Don't

- ❌ Modify third-party code (patch files instead)
- ❌ Mix third-party with project code
- ❌ Ignore license restrictions
- ❌ Rely on unpublished APIs
- ❌ Use unmaintained libraries
- ❌ Copy/paste code from web

## Updating This Directory

When adding new third-party dependencies:

1. **Update this README** with new library info
2. **Document license** - Add LICENSE file copy
3. **Add version file** - Document version used
4. **Update Makefile** - Add build rules
5. **Test** - Ensure full build works
6. **Document usage** - Show how to use in code

## Resources

### Finding Libraries

- **GitHub**: github.com - Most C/C++ libraries
- **Conan**: conan.io - C/C++ package manager
- **vcpkg**: github.com/Microsoft/vcpkg - Windows/Linux packages
- **CPM.cmake**: CPM.cmake - CMake package manager

### Dependency Management Tools

```bash
# Option: Use package manager
conan create . --name=lds --version=1.0

# Option: Use git submodules
git submodule add <repo> third_party/library

# Current: Manual management
# Advantages: Full control, no tool dependencies
```

## License Summary

This project uses:
- **System Libraries**: GPL/proprietary (part of OS)
- **Third-Party Libraries**: [Document actual licenses here]

---

**Purpose**: External dependencies  
**Maintenance**: Keep updated, track versions  
**Caution**: Monitor license compatibility
