#include "StorageMinion.hpp"
#include <stdexcept>
#include <mutex>
#include <shared_mutex>

namespace hrd41
{

StorageMinion::StorageMinion(size_t size_bytes)
    : m_data(size_bytes, 0), m_size(size_bytes)
{
}

void StorageMinion::Read(size_t offset, std::vector<char>& buf)
{
    if (offset + buf.size() > m_size)
    {
        throw std::out_of_range("Read exceeds minion bounds");
    }

    std::shared_lock<std::shared_mutex> lock(m_lock);
    std::copy(m_data.begin() + offset, m_data.begin() + offset + buf.size(), buf.begin());
}

void StorageMinion::Write(size_t offset, const std::vector<char>& buf)
{
    if (offset + buf.size() > m_size)
    {
        throw std::out_of_range("Write exceeds minion bounds");
    }

    std::unique_lock<std::shared_mutex> lock(m_lock);
    std::copy(buf.begin(), buf.end(), m_data.begin() + offset);
    m_offset_sizes[offset] = buf.size();
}

size_t StorageMinion::GetDataSize(size_t offset) const
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
