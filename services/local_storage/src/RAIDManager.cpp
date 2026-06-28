#include "RAIDManager.hpp"
#include "StorageMinion.hpp"
#include "DriverData.hpp"
#include <thread>
#include <future>

namespace hrd41
{

RAIDStorage::RAIDStorage(size_t num_minions, size_t minion_size)
    : IStorage(0), m_minion_size(minion_size)
{
    // Default to hardware concurrency if not specified
    if (num_minions == 0)
    {
        num_minions = std::thread::hardware_concurrency();
        if (num_minions == 0) num_minions = 4;  // Fallback
    }

    m_num_primary = num_minions / 2;
    if (m_num_primary == 0) m_num_primary = 1;

    size_t total_size = num_minions * minion_size;
    m_stripe_size = minion_size / m_num_primary;

    // Create minions
    for (size_t i = 0; i < num_minions; ++i)
    {
        m_minions.push_back(std::make_unique<StorageMinion>(minion_size));
    }
}

size_t RAIDStorage::PrimaryIndex(size_t offset) const
{
    size_t stripe_idx = offset / m_stripe_size;
    return stripe_idx % m_num_primary;
}

size_t RAIDStorage::MirrorIndex(size_t primary_idx) const
{
    return m_num_primary + primary_idx;
}

size_t RAIDStorage::LocalOffset(size_t offset) const
{
    return offset % m_stripe_size;
}

void RAIDStorage::Read(std::shared_ptr<DriverData> data_)
{
    size_t offset = data_->m_offset;
    size_t remaining = data_->m_buffer.size();
    size_t buf_pos = 0;

    try
    {
        // Read data striped across minions
        while (remaining > 0)
        {
            size_t primary_idx = PrimaryIndex(offset);
            size_t local_offset = LocalOffset(offset);
            size_t available_in_minion = m_stripe_size - local_offset;
            size_t to_read = std::min(remaining, available_in_minion);

            std::vector<char> temp_buf(to_read);
            try
            {
                m_minions[primary_idx]->Read(local_offset, temp_buf);
                std::copy(temp_buf.begin(), temp_buf.end(),
                         data_->m_buffer.begin() + buf_pos);
            }
            catch (const std::exception&)
            {
                // Try mirror
                size_t mirror_idx = MirrorIndex(primary_idx);
                m_minions[mirror_idx]->Read(local_offset, temp_buf);
                std::copy(temp_buf.begin(), temp_buf.end(),
                         data_->m_buffer.begin() + buf_pos);
            }

            offset += to_read;
            buf_pos += to_read;
            remaining -= to_read;
        }

        data_->m_status = DriverData::SUCCESS;
    }
    catch (const std::exception&)
    {
        data_->m_status = DriverData::FAILURE;
    }
}

void RAIDStorage::Write(std::shared_ptr<DriverData> data_)
{
    size_t offset = data_->m_offset;
    size_t remaining = data_->m_buffer.size();
    size_t buf_pos = 0;

    try
    {
        // Write data striped across minions
        while (remaining > 0)
        {
            size_t primary_idx = PrimaryIndex(offset);
            size_t mirror_idx = MirrorIndex(primary_idx);
            size_t local_offset = LocalOffset(offset);
            size_t available_in_minion = m_stripe_size - local_offset;
            size_t to_write = std::min(remaining, available_in_minion);

            // Create chunk to write
            std::vector<char> chunk(data_->m_buffer.begin() + buf_pos,
                                   data_->m_buffer.begin() + buf_pos + to_write);

            // Write to primary synchronously
            m_minions[primary_idx]->Write(local_offset, chunk);

            // Write to mirror asynchronously
            auto mirror_future = std::async(std::launch::async,
                [this, mirror_idx, local_offset, chunk]() {
                    try
                    {
                        m_minions[mirror_idx]->Write(local_offset, chunk);
                    }
                    catch (const std::exception&)
                    {
                        // Mirror write failure is non-critical
                    }
                });
            (void)mirror_future;  // Intentionally ignored async result

            offset += to_write;
            buf_pos += to_write;
            remaining -= to_write;
        }

        data_->m_status = DriverData::SUCCESS;
    }
    catch (const std::exception&)
    {
        data_->m_status = DriverData::FAILURE;
    }
}

size_t RAIDStorage::GetDataSize(size_t offset_) const
{
    size_t primary_idx = PrimaryIndex(offset_);
    size_t local_offset = LocalOffset(offset_);

    try
    {
        return m_minions[primary_idx]->GetDataSize(local_offset);
    }
    catch (const std::exception&)
    {
        // Try mirror
        size_t mirror_idx = MirrorIndex(primary_idx);
        try
        {
            return m_minions[mirror_idx]->GetDataSize(local_offset);
        }
        catch (const std::exception&)
        {
            return 0;
        }
    }
}

} // namespace hrd41
