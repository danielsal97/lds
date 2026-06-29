// ============================================================================
// StorageFactory.hpp
// What:  Factory for creating minions and metadata stores
// Why:   Decouples backend selection from RAIDStorage
// How:   Static methods return ready-made collections
// ============================================================================

#ifndef __ILRD_STORAGE_FACTORY_HPP__
#define __ILRD_STORAGE_FACTORY_HPP__

#include "IStorageMinion.hpp"
#include "IMetadataStore.hpp"
#include <memory>
#include <vector>
#include <string>

namespace hrd41
{

enum class MinionBackend
{
    MEMORY,
    FILE
};

class StorageFactory
{
public:
    StorageFactory() = delete;
    StorageFactory(const StorageFactory&) = delete;
    StorageFactory& operator=(const StorageFactory&) = delete;

    // Create minions based on backend type
    // Returns vector of N minion pointers
    static std::vector<std::unique_ptr<IStorageMinion>> CreateMinions(
        MinionBackend backend,
        const std::string& backend_path,
        size_t num_minions,
        size_t minion_size
    );

    // Create metadata store based on backend type
    // FILE backend requires backend_path
    static std::unique_ptr<IMetadataStore> CreateMetadataStore(
        MinionBackend backend,
        const std::string& backend_path
    );
};

} // namespace hrd41

#endif // __ILRD_STORAGE_FACTORY_HPP__
