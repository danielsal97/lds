// ============================================================================
// LocalStorage.hpp
// What:  Concrete storage implementation using local memory buffer
// Why:   Stores data in memory-mapped buffer; called by NBDDriverComm when
//        kernel sends read/write requests
// How:   Receives DriverData with action/offset/length, reads or writes to
//        internal m_storage buffer
// ============================================================================

#ifndef __ILRD_LOCAL_STORAGE__
#define __ILRD_LOCAL_STORAGE__

#include <memory> // shared_ptr

#include "IStorage.hpp"
#include "DriverData.hpp"

namespace hrd41
{
class LocalStorage : public IStorage
{
public:
    explicit LocalStorage(size_t size_);
    LocalStorage(const LocalStorage& o_) = delete;
    LocalStorage& operator=(const LocalStorage& o_) = delete;
    ~LocalStorage() = default;
    
    void Read(std::shared_ptr<DriverData> data_) override;  //throw StorageError
    void Write(std::shared_ptr<DriverData> data_) override;  //throw StorageError

private:
    std::vector<char> m_storage; 
};
    
class LocalStorageError : public StorageError
{
    LocalStorageError(const std::string& msg_);
    ~LocalStorageError();
};   
    
} // namespace hrd41

#endif //__ILRD_LOCAL_STORAGE__
