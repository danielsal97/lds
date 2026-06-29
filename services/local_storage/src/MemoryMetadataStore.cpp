#include "MemoryMetadataStore.hpp"

namespace hrd41
{

std::map<size_t, size_t> MemoryMetadataStore::Load()
{
    // Memory store starts with empty map on every load
    return m_metadata;
}

void MemoryMetadataStore::Save(const std::map<size_t, size_t>& metadata)
{
    m_metadata = metadata;
}

void MemoryMetadataStore::Clear()
{
    m_metadata.clear();
}

} // namespace hrd41
