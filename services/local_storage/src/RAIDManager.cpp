#include "RAIDManager.hpp"
#include "StorageMinionMemory.hpp"
#include "StorageMinionFile.hpp"
#include "DriverData.hpp"
#include "logger.hpp"

#include <algorithm>
#include <stdexcept>
#include <mutex>
#include <shared_mutex>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace hrd41
{


RAIDStorage::RAIDStorage(size_t num_minions, size_t minion_size, MinionBackend backend, const std::string& backend_path)
    : IStorage(num_minions * minion_size),
      m_minion_size(minion_size),
      m_backend_path(backend_path)
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

    // Create minions based on backend type
    if (backend == MinionBackend::FILE && backend_path.empty())
    {
        throw std::runtime_error("FILE backend requires backend_path to be specified");
    }

    // Create backend directory if needed
    if (backend == MinionBackend::FILE)
    {
        std::filesystem::create_directories(backend_path);
    }

    for (size_t i = 0; i < num_minions; ++i)
    {
        switch (backend)
        {
            case MinionBackend::MEMORY:
                m_minions.push_back(std::make_unique<StorageMinionMemory>(minion_size));
                break;
            case MinionBackend::FILE:
            {
                std::string file_path = backend_path + "/minion" + std::to_string(i) + ".img";
                m_minions.push_back(std::make_unique<StorageMinionFile>(file_path, minion_size));
                break;
            }
            default:
                throw std::runtime_error("Unknown minion backend");
        }
    }

    // Load metadata after minions are created
    LoadMetadata();

    std::string backend_str = (backend == MinionBackend::MEMORY) ? "memory" : "file";
    logger->Write("[RAID] Initialized with " + std::to_string(num_minions) + " minions (" +
                  backend_str + " backend), " +
                  std::to_string(m_num_primary) + " primary + " +
                  std::to_string(m_num_primary) + " mirror, stripe_size=" +
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

        // Persist metadata to disk
        SaveMetadata();

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

void RAIDStorage::SaveMetadata() const
{
    // Only save if we have a backend path (FILE backend)
    if (m_backend_path.empty())
    {
        return;
    }

    std::string metadata_path = m_backend_path + "/raid_metadata.dat";

    std::shared_lock<std::shared_mutex> lock(m_offsets_lock);

    std::ofstream file(metadata_path, std::ios::binary);
    if (!file.is_open())
    {
        auto logger = Singleton<Logger>::GetInstance();
        logger->Write("[RAID] Failed to open metadata file for writing: " + metadata_path, Logger::ERROR);
        return;
    }

    for (const auto& [offset, size] : m_offset_sizes)
    {
        file.write(reinterpret_cast<const char*>(&offset), sizeof(size_t));
        file.write(reinterpret_cast<const char*>(&size), sizeof(size_t));
    }

    file.close();
    auto logger = Singleton<Logger>::GetInstance();
    logger->Write("[RAID] Metadata saved to " + metadata_path + " (" +
                  std::to_string(m_offset_sizes.size()) + " entries)", Logger::DEBUG);
}

void RAIDStorage::LoadMetadata()
{
    auto logger = Singleton<Logger>::GetInstance();

    // Only load if we have a backend path (FILE backend)
    if (m_backend_path.empty())
    {
        logger->Write("[RAID] No backend path; skipping metadata load", Logger::DEBUG);
        return;
    }

    std::string metadata_path = m_backend_path + "/raid_metadata.dat";

    // If metadata file doesn't exist, start with empty map
    if (!std::filesystem::exists(metadata_path))
    {
        logger->Write("[RAID] Metadata file not found: " + metadata_path + " (starting fresh)", Logger::INFO);
        return;
    }

    std::ifstream file(metadata_path, std::ios::binary);
    if (!file.is_open())
    {
        logger->Write("[RAID] Failed to open metadata file for reading: " + metadata_path, Logger::ERROR);
        return;
    }

    std::unique_lock<std::shared_mutex> lock(m_offsets_lock);
    m_offset_sizes.clear();

    size_t offset, size;
    while (file.read(reinterpret_cast<char*>(&offset), sizeof(size_t)))
    {
        if (!file.read(reinterpret_cast<char*>(&size), sizeof(size_t)))
        {
            logger->Write("[RAID] Corrupted metadata file: incomplete entry", Logger::ERROR);
            m_offset_sizes.clear();
            return;
        }
        m_offset_sizes[offset] = size;
    }

    logger->Write("[RAID] Metadata loaded from " + metadata_path + " (" +
                  std::to_string(m_offset_sizes.size()) + " entries)", Logger::INFO);
}

} // namespace hrd41