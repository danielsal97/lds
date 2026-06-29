#include "StorageMinionFile.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdexcept>
#include <mutex>
#include <shared_mutex>
#include <algorithm>

namespace hrd41
{

StorageMinionFile::StorageMinionFile(const std::string& file_path, size_t size_bytes)
    : m_file_path(file_path), m_size(size_bytes)
{
    // Open or create the file
    m_fd = open(file_path.c_str(), O_RDWR | O_CREAT, 0644);
    if (m_fd < 0)
    {
        throw std::runtime_error("Failed to open/create file: " + file_path);
    }

    // Check if file exists and has content
    struct stat st;
    if (fstat(m_fd, &st) < 0)
    {
        close(m_fd);
        throw std::runtime_error("Failed to stat file: " + file_path);
    }

    // If file is empty, pre-allocate space
    if (st.st_size == 0)
    {
        if (ftruncate(m_fd, size_bytes) < 0)
        {
            close(m_fd);
            throw std::runtime_error("Failed to allocate file: " + file_path);
        }
    }
}

StorageMinionFile::~StorageMinionFile()
{
    if (m_fd >= 0)
    {
        close(m_fd);
    }
}

void StorageMinionFile::Read(size_t offset, std::vector<char>& buf)
{
    if (offset + buf.size() > m_size)
    {
        throw std::out_of_range("Read exceeds minion bounds");
    }

    std::shared_lock<std::shared_mutex> lock(m_lock);

    ssize_t n = pread(m_fd, buf.data(), buf.size(), offset);
    if (n < 0)
    {
        throw std::runtime_error("pread failed");
    }
    if (static_cast<size_t>(n) != buf.size())
    {
        throw std::runtime_error("pread returned fewer bytes than requested");
    }
}

void StorageMinionFile::Write(size_t offset, const std::vector<char>& buf)
{
    if (offset + buf.size() > m_size)
    {
        throw std::out_of_range("Write exceeds minion bounds");
    }

    std::unique_lock<std::shared_mutex> lock(m_lock);

    ssize_t n = pwrite(m_fd, buf.data(), buf.size(), offset);
    if (n < 0)
    {
        throw std::runtime_error("pwrite failed");
    }
    if (static_cast<size_t>(n) != buf.size())
    {
        throw std::runtime_error("pwrite wrote fewer bytes than requested");
    }

    m_offset_sizes[offset] = buf.size();
}

size_t StorageMinionFile::GetDataSize(size_t offset) const
{
    std::shared_lock<std::shared_mutex> lock(m_lock);
    auto it = m_offset_sizes.find(offset);
    if (it != m_offset_sizes.end())
    {
        return it->second;
    }
    return 0;
}

} // namespace hrd41
