// ============================================================================
// IStorage.hpp
// What:  Interface for storage implementations (read/write data)
// Why:   Abstracts storage backend; allows swapping LocalStorage for other
//        implementations (e.g., remote storage, database) without changing callers
// How:   Subclass IStorage, implement Read(DriverData) and Write(DriverData),
//        LocalStorage uses local memory buffer
// ============================================================================

#ifndef __ILRD_ISTORAGE_HPP__
#define __ILRD_ISTORAGE_HPP__

#include <memory> //shared_ptr
#include <stdexcept> // runtime_error

#include "DriverData.hpp"

namespace hrd41
{
    
class IStorage
{
public:
    explicit IStorage(size_t size_);
    IStorage(const IStorage& o_) = delete;
    IStorage& operator=(const IStorage& o_) = delete;
    virtual ~IStorage() = default; 

    virtual void Read(std::shared_ptr<DriverData> data_) = 0;  //throw StorageError
    virtual void Write(std::shared_ptr<DriverData> data_) = 0; //throw StorageError
    
private:
    size_t size_;
};

class StorageError: public std::runtime_error
{
    StorageError(const std::string& msg_);
    virtual ~StorageError() = 0;
};

} // hrd41

#endif  // __ILRD_ISTORAGE_HPP__

