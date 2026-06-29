// ============================================================================
// FileMetadataStore.hpp
// What:  File-backed metadata storage (persistent)
// Why:   Metadata survives server restart
// How:   Serializes offset→size map to raid_metadata.dat binary file
// ============================================================================

#ifndef __ILRD_FILE_METADATA_STORE_HPP__
#define __ILRD_FILE_METADATA_STORE_HPP__

#include "IMetadataStore.hpp"
#include <string>

namespace hrd41
{

class FileMetadataStore : public IMetadataStore
{
public:
    // metadata_path: directory where raid_metadata.dat will be stored
    explicit FileMetadataStore(const std::string& metadata_path);
    FileMetadataStore(const FileMetadataStore&) = delete;
    FileMetadataStore& operator=(const FileMetadataStore&) = delete;
    ~FileMetadataStore() override = default;

    std::map<size_t, size_t> Load() override;
    void Save(const std::map<size_t, size_t>& metadata) override;
    void Clear() override;

private:
    std::string m_file_path;  // Full path to raid_metadata.dat
};

} // namespace hrd41

#endif // __ILRD_FILE_METADATA_STORE_HPP__
