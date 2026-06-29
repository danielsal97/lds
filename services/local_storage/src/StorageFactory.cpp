#include "StorageFactory.hpp"
#include "StorageMinionMemory.hpp"
#include "StorageMinionFile.hpp"
#include "MemoryMetadataStore.hpp"
#include "FileMetadataStore.hpp"
#include "logger.hpp"
#include "singelton.hpp"
#include <stdexcept>
#include <filesystem>

namespace hrd41
{

std::vector<std::unique_ptr<IStorageMinion>> StorageFactory::CreateMinions(
    MinionBackend backend,
    const std::string& backend_path,
    size_t num_minions,
    size_t minion_size
)
{
    auto logger = Singleton<Logger>::GetInstance();
    std::vector<std::unique_ptr<IStorageMinion>> minions;

    if (num_minions == 0)
    {
        throw std::runtime_error("StorageFactory requires at least 1 minion");
    }

    if (backend == MinionBackend::FILE && backend_path.empty())
    {
        throw std::runtime_error("FILE backend requires backend_path");
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
                minions.push_back(std::make_unique<StorageMinionMemory>(minion_size));
                break;

            case MinionBackend::FILE:
            {
                std::string file_path = backend_path + "/minion" + std::to_string(i) + ".img";
                minions.push_back(std::make_unique<StorageMinionFile>(file_path, minion_size));
                break;
            }

            default:
                throw std::runtime_error("Unknown minion backend");
        }
    }

    std::string backend_str = (backend == MinionBackend::MEMORY) ? "memory" : "file";
    logger->Write("[FACTORY] Created " + std::to_string(num_minions) + " minions (" +
                  backend_str + " backend)", Logger::INFO);

    return minions;
}

std::unique_ptr<IMetadataStore> StorageFactory::CreateMetadataStore(
    MinionBackend backend,
    const std::string& backend_path
)
{
    auto logger = Singleton<Logger>::GetInstance();

    switch (backend)
    {
        case MinionBackend::MEMORY:
            logger->Write("[FACTORY] Created MemoryMetadataStore (volatile)", Logger::INFO);
            return std::make_unique<MemoryMetadataStore>();

        case MinionBackend::FILE:
            if (backend_path.empty())
            {
                throw std::runtime_error("FILE metadata store requires backend_path");
            }
            logger->Write("[FACTORY] Created FileMetadataStore at " + backend_path, Logger::INFO);
            return std::make_unique<FileMetadataStore>(backend_path);

        default:
            throw std::runtime_error("Unknown metadata store backend");
    }
}

} // namespace hrd41
