#include "IStorage.hpp"

namespace hrd41
{

IStorage::IStorage(size_t size)
    : size_(size)
{
}

StorageError::StorageError(const std::string& msg_)
    : std::runtime_error(msg_)
{
}

StorageError::~StorageError() = default;

} // namespace hrd41
