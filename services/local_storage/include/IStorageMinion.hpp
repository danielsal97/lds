// ============================================================================
// IStorageMinion.hpp
// What:  Abstract storage backend for RAID minions
// Why:   Enables pluggable storage (memory / file / NBD / TCP / etc.)
//        RAIDStorage works with any backend without modification
// How:   Implements Read/Write/GetDataSize — three operations only
// ============================================================================

#ifndef __ILRD_ISTORAGE_MINION_HPP__
#define __ILRD_ISTORAGE_MINION_HPP__

#include <vector>
#include <cstddef>

namespace hrd41
{

class IStorageMinion
{
public:
    virtual ~IStorageMinion() = default;

    virtual void Read(size_t offset, std::vector<char>& buffer) = 0;
    virtual void Write(size_t offset, const std::vector<char>& buffer) = 0;
    virtual size_t GetDataSize(size_t offset) const = 0;
    virtual size_t Size() const = 0;  // Total size of this minion
};

} // namespace hrd41

#endif // __ILRD_ISTORAGE_MINION_HPP__
