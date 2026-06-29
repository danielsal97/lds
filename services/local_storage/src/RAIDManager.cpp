#include "RAIDManager.hpp"
#include "StorageMinion.hpp"
#include "DriverData.hpp"
#include "logger.hpp"

#include <algorithm>
#include <stdexcept>
#include <mutex>
#include <shared_mutex>

namespace hrd41
{


RAIDStorage::RAIDStorage(size_t num_minions, size_t minion_size)
    : IStorage(num_minions * minion_size),
      m_minion_size(minion_size)
{
    auto logger = Singleton<Logger>::GetInstance();

    if (num_minions == 0)
    {
        throw std::runtime_error("RAIDStorage requires at least 2 minions");
    }

    if (num_minions % 2 != 0)
    {
        throw std::runtime_error("RAIDStorage requires an even number of minions");
    }

    m_num_primary = num_minions / 2;

    if (m_num_primary == 0)
    {
        throw std::runtime_error("RAIDStorage requires at least one primary minion");
    }

    m_stripe_size = minion_size;

    for (size_t i = 0; i < num_minions; ++i)
    {
        m_minions.push_back(std::make_unique<StorageMinion>(minion_size));
    }

    logger->Write("[RAID] Initialized with " + std::to_string(num_minions) + " minions " +
                  "(" + std::to_string(m_num_primary) + " primary + " +
                  std::to_string(m_num_primary) + " mirror), stripe_size=" +
                  std::to_string(m_stripe_size) + " bytes", Logger::INFO);
}

size_t RAIDStorage::PrimaryIndex(size_t offset) const
{
    size_t stripe_index = offset / m_stripe_size;
    return stripe_index % m_num_primary;
}

size_t RAIDStorage::MirrorIndex(size_t primary_idx) const
{
    return m_num_primary + primary_idx;
}

size_t RAIDStorage::LocalOffset(size_t offset) const
{
    size_t stripe_index = offset / m_stripe_size;
    size_t stripe_group = stripe_index / m_num_primary;

    return stripe_group * m_stripe_size + (offset % m_stripe_size);
}

void RAIDStorage::Read(std::shared_ptr<DriverData> data_)
{
    auto logger = Singleton<Logger>::GetInstance();
    size_t offset = data_->m_offset;
    size_t remaining = data_->m_len;
    size_t buf_pos = 0;

    data_->m_buffer.resize(data_->m_len);

    logger->Write("[RAID-READ] offset=" + std::to_string(data_->m_offset) +
                  " len=" + std::to_string(data_->m_len), Logger::DEBUG);

    try
    {
        while (remaining > 0)
        {
            size_t primary_idx = PrimaryIndex(offset);
            size_t mirror_idx = MirrorIndex(primary_idx);
            size_t local_offset = LocalOffset(offset);

            size_t available_in_stripe = m_stripe_size - (offset % m_stripe_size);
            size_t to_read = std::min(remaining, available_in_stripe);

            std::vector<char> temp_buf(to_read);

            try
            {
                m_minions[primary_idx]->Read(local_offset, temp_buf);
                logger->Write("[RAID-READ] offset=" + std::to_string(offset) +
                              " → primary[" + std::to_string(primary_idx) + "]", Logger::DEBUG);
            }
            catch (const std::exception&)
            {
                m_minions[mirror_idx]->Read(local_offset, temp_buf);
                logger->Write("[RAID-READ] offset=" + std::to_string(offset) +
                              " → mirror[" + std::to_string(mirror_idx) + "] (primary failed)", Logger::INFO);
            }

            std::copy(
                temp_buf.begin(),
                temp_buf.end(),
                data_->m_buffer.begin() + buf_pos
            );

            offset += to_read;
            buf_pos += to_read;
            remaining -= to_read;
        }

        data_->m_status = DriverData::SUCCESS;
    }
    catch (const std::exception& e)
    {
        logger->Write("[RAID-READ] FAILED: " + std::string(e.what()), Logger::ERROR);
        data_->m_status = DriverData::FAILURE;
    }
}

void RAIDStorage::Write(std::shared_ptr<DriverData> data_)
{
    auto logger = Singleton<Logger>::GetInstance();
    size_t original_offset = data_->m_offset;
    size_t original_size = data_->m_buffer.size();

    size_t offset = data_->m_offset;
    size_t remaining = data_->m_buffer.size();
    size_t buf_pos = 0;

    logger->Write("[RAID-WRITE] offset=" + std::to_string(original_offset) +
                  " len=" + std::to_string(original_size), Logger::DEBUG);

    try
    {
        while (remaining > 0)
        {
            size_t primary_idx = PrimaryIndex(offset);
            size_t mirror_idx = MirrorIndex(primary_idx);
            size_t local_offset = LocalOffset(offset);

            size_t available_in_stripe = m_stripe_size - (offset % m_stripe_size);
            size_t to_write = std::min(remaining, available_in_stripe);

            std::vector<char> chunk(
                data_->m_buffer.begin() + buf_pos,
                data_->m_buffer.begin() + buf_pos + to_write
            );

            m_minions[primary_idx]->Write(local_offset, chunk);
            m_minions[mirror_idx]->Write(local_offset, chunk);

            logger->Write("[RAID-WRITE] offset=" + std::to_string(offset) +
                          " → primary[" + std::to_string(primary_idx) + "] + mirror[" +
                          std::to_string(mirror_idx) + "] (" + std::to_string(to_write) + " bytes)", Logger::DEBUG);

            offset += to_write;
            buf_pos += to_write;
            remaining -= to_write;
        }

        {
            std::unique_lock<std::shared_mutex> lock(m_offsets_lock);
            m_offset_sizes[original_offset] = original_size;
        }

        logger->Write("[RAID-WRITE] offset=" + std::to_string(original_offset) +
                      " SUCCESS (" + std::to_string(original_size) + " bytes)", Logger::INFO);

        data_->m_status = DriverData::SUCCESS;
    }
    catch (const std::exception& e)
    {
        logger->Write("[RAID-WRITE] offset=" + std::to_string(original_offset) +
                      " FAILED: " + std::string(e.what()), Logger::ERROR);
        data_->m_status = DriverData::FAILURE;
    }
}

size_t RAIDStorage::GetDataSize(size_t offset_) const
{
    std::shared_lock<std::shared_mutex> lock(m_offsets_lock);

    auto it = m_offset_sizes.find(offset_);
    if (it == m_offset_sizes.end())
    {
        return 0;
    }

    return it->second;
}

std::vector<std::pair<size_t, size_t>> RAIDStorage::ListOffsets() const
{
    std::shared_lock<std::shared_mutex> lock(m_offsets_lock);

    std::vector<std::pair<size_t, size_t>> result;

    for (const auto& [offset, size] : m_offset_sizes)
    {
        result.push_back({offset, size});
    }

    return result;
}

} // namespace hrd41