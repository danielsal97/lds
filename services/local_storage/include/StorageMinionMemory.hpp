// ============================================================================
// StorageMinionMemory.hpp
// What:  In-memory implementation of IStorageMinion
// Why:   Fast, suitable for testing; will be one of many backend choices
// How:   Uses std::vector<char> for storage, std::map for offset tracking
// ============================================================================

#ifndef __ILRD_STORAGE_MINION_MEMORY_HPP__
#define __ILRD_STORAGE_MINION_MEMORY_HPP__

#include <vector>
#include <shared_mutex>
#include <map>
#include <algorithm>
#include "IStorageMinion.hpp"

namespace hrd41
{

class StorageMinionMemory : public IStorageMinion
{
public:
    explicit StorageMinionMemory(size_t size_bytes = 5 * 1024 * 1024);
    StorageMinionMemory(const StorageMinionMemory&) = delete;
    StorageMinionMemory& operator=(const StorageMinionMemory&) = delete;
    ~StorageMinionMemory() = default;

    void Read(size_t offset, std::vector<char>& buf) override;
    void Write(size_t offset, const std::vector<char>& buf) override;
    size_t GetDataSize(size_t offset) const override;
    size_t Size() const { return m_size; }

private:
    std::vector<char> m_data;
    mutable std::shared_mutex m_lock;  // RWLock: concurrent reads, exclusive writes
    size_t m_size;
    std::map<size_t, size_t> m_offset_sizes;  // Track what's been written
};

} // namespace hrd41

#endif // __ILRD_STORAGE_MINION_MEMORY_HPP__
