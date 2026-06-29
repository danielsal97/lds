// ============================================================================
// StorageMinionFile.hpp
// What:  File-backed implementation of IStorageMinion
// Why:   Persistent storage using files; enables RAID across disk storage
// How:   Uses pread/pwrite for non-blocking I/O to single .img file
// ============================================================================

#ifndef __ILRD_STORAGE_MINION_FILE_HPP__
#define __ILRD_STORAGE_MINION_FILE_HPP__

#include <string>
#include <map>
#include <shared_mutex>
#include "IStorageMinion.hpp"

namespace hrd41
{

class StorageMinionFile : public IStorageMinion
{
public:
    // Open or create a file-backed minion at file_path with given size
    explicit StorageMinionFile(const std::string& file_path, size_t size_bytes);
    StorageMinionFile(const StorageMinionFile&) = delete;
    StorageMinionFile& operator=(const StorageMinionFile&) = delete;
    ~StorageMinionFile();

    void Read(size_t offset, std::vector<char>& buf) override;
    void Write(size_t offset, const std::vector<char>& buf) override;
    size_t GetDataSize(size_t offset) const override;
    size_t Size() const { return m_size; }

private:
    int m_fd;                               // File descriptor for .img file
    std::string m_file_path;
    size_t m_size;
    std::map<size_t, size_t> m_offset_sizes; // Track what's been written
    mutable std::shared_mutex m_lock;        // RWLock for concurrent access
};

} // namespace hrd41

#endif // __ILRD_STORAGE_MINION_FILE_HPP__
