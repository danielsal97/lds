// ============================================================================
// IMetadataStore.hpp
// What:  Abstract interface for RAID logical metadata persistence
// Why:   Decouples metadata persistence from RAID logic
// How:   Load/Save the offset→size map without knowing storage backend
// ============================================================================

#ifndef __ILRD_METADATA_STORE_HPP__
#define __ILRD_METADATA_STORE_HPP__

#include <map>
#include <cstddef>

namespace hrd41
{

class IMetadataStore
{
public:
    virtual ~IMetadataStore() = default;

    // Load all metadata from persistent storage
    // Returns map of offset → size
    virtual std::map<size_t, size_t> Load() = 0;

    // Save entire metadata snapshot to persistent storage
    // RAIDStorage owns the logical state; store just persists it
    virtual void Save(const std::map<size_t, size_t>& metadata) = 0;

    // Clear all metadata
    virtual void Clear() = 0;
};

} // namespace hrd41

#endif // __ILRD_METADATA_STORE_HPP__
