// ============================================================================
// MemoryMetadataStore.hpp
// What:  In-memory metadata storage (volatile)
// Why:   Fast metadata access without I/O; data lost on restart
// How:   Keeps map in memory; Load returns empty on first run
// ============================================================================

#ifndef __ILRD_MEMORY_METADATA_STORE_HPP__
#define __ILRD_MEMORY_METADATA_STORE_HPP__

#include "IMetadataStore.hpp"
#include <map>

namespace hrd41
{

class MemoryMetadataStore : public IMetadataStore
{
public:
    MemoryMetadataStore() = default;
    MemoryMetadataStore(const MemoryMetadataStore&) = delete;
    MemoryMetadataStore& operator=(const MemoryMetadataStore&) = delete;
    ~MemoryMetadataStore() override = default;

    std::map<size_t, size_t> Load() override;
    void Save(const std::map<size_t, size_t>& metadata) override;
    void Clear() override;

private:
    std::map<size_t, size_t> m_metadata;
};

} // namespace hrd41

#endif // __ILRD_MEMORY_METADATA_STORE_HPP__
