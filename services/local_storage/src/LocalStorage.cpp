#include "LocalStorage.hpp"
#include "IStorage.hpp"
#include <cstddef>
#include <vector>

namespace hrd41
{
LocalStorage::LocalStorage(size_t size_) : IStorage(size_), m_storage(std::vector<char>(size_))
{
}

void LocalStorage::Read(std::shared_ptr<DriverData> data_)
{
    
    if (data_->m_offset + data_->m_buffer.size() > m_storage.size())
    {
        throw std::out_of_range("Read exceeds storage bounds");
    }

    std::copy(m_storage.begin() + data_->m_offset,
              m_storage.begin() + data_->m_offset + data_->m_buffer.size(),
              data_->m_buffer.begin());
}

void LocalStorage::Write(std::shared_ptr<DriverData> data_)
{
    if (data_->m_offset> m_storage.size())
    {
        throw std::out_of_range("Write exceeds storage bounds");
    }

    std::copy(data_->m_buffer.begin(),
              data_->m_buffer.begin() + data_->m_buffer.size(),
              m_storage.begin() + data_->m_offset);

}} // namespace hrd41
