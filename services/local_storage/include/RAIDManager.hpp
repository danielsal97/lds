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
#include "IStorage.hpp"
#include "StorageMinion.hpp"

namespace hrd41
{

class RAIDStorage : public IStorage
{
public:
    // num_minions defaults to hardware_concurrency, minion_size defaults to 5MB
    explicit RAIDStorage(size_t num_minions = 0,
                         size_t minion_size = 5 * 1024 * 1024);
    RAIDStorage(const RAIDStorage&) = delete;
    RAIDStorage& operator=(const RAIDStorage&) = delete;
    ~RAIDStorage() = default;

    void Read(std::shared_ptr<DriverData> data_) override;
    void Write(std::shared_ptr<DriverData> data_) override;
    size_t GetDataSize(size_t offset_) const override;

    size_t GetNumMinions() const { return m_minions.size(); }
    size_t GetMinionsPerSet() const { return m_num_primary; }

private:
    std::vector<std::unique_ptr<StorageMinion>> m_minions;
    size_t m_stripe_size;      // bytes per stripe chunk
    size_t m_num_primary;      // N/2 (number of primary minions)
    size_t m_minion_size;      // size of each minion

    size_t PrimaryIndex(size_t offset) const;
    size_t MirrorIndex(size_t primary_idx) const;
    size_t LocalOffset(size_t offset) const;
};

} // namespace hrd41

#endif // __ILRD_RAID_MANAGER_HPP__
