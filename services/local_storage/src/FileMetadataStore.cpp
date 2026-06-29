#include "FileMetadataStore.hpp"
#include "logger.hpp"
#include "singelton.hpp"
#include <fstream>
#include <filesystem>

namespace hrd41
{

FileMetadataStore::FileMetadataStore(const std::string& metadata_path)
    : m_file_path(metadata_path + "/raid_metadata.dat")
{
    // Ensure directory exists
    std::filesystem::create_directories(metadata_path);
}

std::map<size_t, size_t> FileMetadataStore::Load()
{
    auto logger = Singleton<Logger>::GetInstance();
    std::map<size_t, size_t> metadata;

    // If file doesn't exist, start with empty map (first run)
    if (!std::filesystem::exists(m_file_path))
    {
        logger->Write("[METADATA] File not found: " + m_file_path + " (starting fresh)", Logger::INFO);
        return metadata;
    }

    std::ifstream file(m_file_path, std::ios::binary);
    if (!file.is_open())
    {
        logger->Write("[METADATA] Failed to open: " + m_file_path, Logger::ERROR);
        return metadata;
    }

    size_t offset, size;
    while (file.read(reinterpret_cast<char*>(&offset), sizeof(size_t)))
    {
        if (!file.read(reinterpret_cast<char*>(&size), sizeof(size_t)))
        {
            logger->Write("[METADATA] Corrupted file: incomplete entry", Logger::ERROR);
            metadata.clear();
            return metadata;
        }
        metadata[offset] = size;
    }

    logger->Write("[METADATA] Loaded from " + m_file_path + " (" +
                  std::to_string(metadata.size()) + " entries)", Logger::INFO);
    return metadata;
}

void FileMetadataStore::Save(const std::map<size_t, size_t>& metadata)
{
    auto logger = Singleton<Logger>::GetInstance();

    std::ofstream file(m_file_path, std::ios::binary);
    if (!file.is_open())
    {
        logger->Write("[METADATA] Failed to open for writing: " + m_file_path, Logger::ERROR);
        return;
    }

    for (const auto& [offset, size] : metadata)
    {
        file.write(reinterpret_cast<const char*>(&offset), sizeof(size_t));
        file.write(reinterpret_cast<const char*>(&size), sizeof(size_t));
    }

    file.close();
    logger->Write("[METADATA] Saved to " + m_file_path + " (" +
                  std::to_string(metadata.size()) + " entries)", Logger::DEBUG);
}

void FileMetadataStore::Clear()
{
    auto logger = Singleton<Logger>::GetInstance();

    if (std::filesystem::exists(m_file_path))
    {
        std::filesystem::remove(m_file_path);
        logger->Write("[METADATA] Cleared: " + m_file_path, Logger::INFO);
    }
}

} // namespace hrd41
