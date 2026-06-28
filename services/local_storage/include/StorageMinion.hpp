// ============================================================================
// StorageMinion.hpp
// What:  Individual storage unit for RAID architecture (default 5MB each)
// Why:   Provides independent, concurrent access with per-unit locking;
//        allows RAID 01 (stripe+mirror) without single-lock bottleneck
// How:   Each minion has its own mutex + buffer; RAID layer distributes
//        offsets to minions based on stripe algorithm
// ============================================================================

#ifndef __ILRD_STORAGE_MINION_HPP__
#define __ILRD_STORAGE_MINION_HPP__

#include <vector>
#include <shared_mutex>
#include <map>
#include <cstring>

namespace hrd41
{

class StorageMinion
{
public:
    explicit StorageMinion(size_t size_bytes = 5 * 1024 * 1024);
    StorageMinion(const StorageMinion&) = delete;
    StorageMinion& operator=(const StorageMinion&) = delete;
    ~StorageMinion() = default;

    void Read(size_t offset, std::vector<char>& buf);
    void Write(size_t offset, const std::vector<char>& buf);
    size_t GetDataSize(size_t offset) const;
    size_t Size() const { return m_size; }

private:
    std::vector<char> m_data;
    mutable std::shared_mutex m_lock;  // RWLock: concurrent reads, exclusive writes
    size_t m_size;
    std::map<size_t, size_t> m_offset_sizes;  // Track what's been written
};

} // namespace hrd41

#endif // __ILRD_STORAGE_MINION_HPP__
