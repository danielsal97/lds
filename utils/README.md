# Utilities & Helpers (`/utils`)

## Purpose

This directory contains **lightweight utility functions and helper classes** that support the main framework. These are general-purpose tools used across the codebase.

## Status

🔄 **Actively Used** - Contains helper functions and utility implementations

## Typical Contents

This directory may include:

- **String utilities** - parsing, formatting, manipulation
- **File utilities** - path operations, directory handling
- **Conversion functions** - type conversions, serialization
- **Math utilities** - common mathematical operations
- **Memory utilities** - allocation wrappers, smart pointers
- **Time utilities** - timing, scheduling helpers

## Relationship to `/utilities`

| Directory | Purpose | Scope |
|-----------|---------|-------|
| `/utils` | Lightweight helper functions | Small, reusable utilities |
| `/utilities` | Core framework components | Large, central frameworks |

## Common Patterns

### Example: String Utilities

```cpp
// utils/string_utils.hpp
namespace StringUtils {
    std::string trim(const std::string& str);
    std::vector<std::string> split(const std::string& str, char delimiter);
    std::string toUpper(const std::string& str);
    std::string toLower(const std::string& str);
}
```

### Example: File Utilities

```cpp
// utils/file_utils.hpp
namespace FileUtils {
    bool fileExists(const std::string& path);
    std::vector<std::string> listDirectory(const std::string& path);
    std::string getBasename(const std::string& path);
    std::string getDirectory(const std::string& path);
}
```

### Example: Convert Utilities

```cpp
// utils/convert.hpp
namespace Convert {
    int toInt(const std::string& str);
    std::string toString(int value);
    bool toBoolean(const std::string& str);
}
```

## Integration with Framework

```
Framework Components (/utilities)
    ↓ uses
Utility Functions (/utils)
    ↓ perform
Basic operations
```

## When to Use

### Use `/utils` for:
- ✅ Simple, focused functionality
- ✅ Reusable across multiple components
- ✅ Not a complete framework
- ✅ Stateless helper functions

### Use `/utilities` for:
- ✅ Complex framework components
- ✅ Components with state/configuration
- ✅ Central services (Logger, Factory)
- ✅ Design pattern implementations

## Testing

Helper utilities should have minimal tests since they're simple. Use them in integration tests to validate complex workflows.

## Adding New Utilities

1. Create new header in `/utils/`
2. Keep implementation simple and focused
3. Make functions reusable
4. Document with clear examples

Example structure:
```cpp
// utils/my_utility.hpp
#pragma once

namespace MyUtility {
    // Forward declaration of functions
    std::string doSomething(const std::string& input);
    int calculateValue(int a, int b);
}
```

## Best Practices

### ✅ Good Utility Functions

```cpp
// Clear purpose
std::string extractFilename(const std::string& path);

// Focused functionality
std::vector<std::string> parseCSV(const std::string& line);

// Reusable across project
bool isValidIP(const std::string& ip);
```

### ❌ Avoid

```cpp
// Too complex (should be in /utilities as framework)
class DatabaseConnectionPool { ... };

// Side effects (should be explicit)
void calculateAndLogResult(int a, int b);

// Broad scope (too many responsibilities)
class GeneralUtility { 
    void something();
    void somethingElse();
    void anotherThing();
};
```

---

**Purpose**: General-purpose utility functions  
**Status**: 🔄 Active development  
**Complexity**: Low to Medium
