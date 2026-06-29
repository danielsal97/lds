// ============================================================================
// RAIDManager.hpp
// What:  RAID 01 storage layer using multiple StorageMinions
// Why:   Distributes load across minions (striping) + mirrors for durability;
//        eliminates single-lock bottleneck of LocalStorage
// How:   N minions split into N/2 primary + N/2 mirror; offset → primary via
//        stripe index; write both, read primary (fallback to mirror on error)
// ============================================================================

#ifndef __ILRD_RAID_MANAGER_HPP__
#define __ILRD_RAID_MANAGER_HPP__

#include <memory>
#include <vector>
#include <map>
#include <string>
#include <shared_mutex>

#include "IStorage.hpp"
#include "IStorageMinion.hpp"

namespace hrd41
{

enum class MinionBackend
{
    MEMORY,
    FILE
};

class RAIDStorage : public IStorage
{
public:
    explicit RAIDStorage(size_t num_minions = 4,
                         size_t minion_size = 5 * 1024 * 1024,
                         MinionBackend backend = MinionBackend::MEMORY,
                         const std::string& backend_path = "");

    RAIDStorage(const RAIDStorage&) = delete;
    RAIDStorage& operator=(const RAIDStorage&) = delete;
    ~RAIDStorage() override = default;

    void Read(std::shared_ptr<DriverData> data_) override;
    void Write(std::shared_ptr<DriverData> data_) override;
    size_t GetDataSize(size_t offset_) const override;
    std::vector<std::pair<size_t, size_t>> ListOffsets() const override;

    size_t GetNumMinions() const;
    size_t GetMinionsPerSet() const;

private:
    std::vector<std::unique_ptr<IStorageMinion>> m_minions;

    size_t m_stripe_size;
    size_t m_num_primary;
    size_t m_minion_size;
    std::string m_backend_path;

    std::map<size_t, size_t> m_offset_sizes;
    mutable std::shared_mutex m_offsets_lock;

    void SaveMetadata() const;
    void LoadMetadata();

    size_t PrimaryIndex(size_t offset) const;
    size_t MirrorIndex(size_t primary_idx) const;
    size_t LocalOffset(size_t offset) const;
};

} // namespace hrd41

#endif // __ILRD_RAID_MANAGER_HPP__