// ============================================================================
// DriverData.hpp
// What:  Data container for requests flowing between driver and storage
// Why:   Standardizes request format so all components work with same data
//        structure regardless of protocol (NBD encodes/decodes to this)
// How:   Contains ActionType (READ/WRITE/etc), offset, length, buffer,
//        handle (to match reply to request), status
// ============================================================================

#ifndef __ILRD_DRIVER_DATA_HPP__
#define __ILRD_DRIVER_DATA_HPP__

#include <cstddef> // size_t
#include <vector>  //vector

namespace hrd41
{
struct DriverData
{
  enum ActionType
  {
    READ,
    WRITE,
    DISCONNECT,
    FLUSH,
    TRIM
  };
  enum StatusType
  {
    SUCCESS,
    FAILURE
  };

  explicit DriverData(ActionType type_, size_t handle_, size_t offset_,
                      size_t nBytes_, StatusType status_ = SUCCESS);

  DriverData(const DriverData& other_) = default;
  DriverData& operator=(const DriverData& other_) = default;
  ~DriverData() = default;

  ActionType m_type; // read, write, disconnect
  size_t m_handle;   // unique identefer to match reply to request
  size_t m_offset;
  size_t m_len;
  StatusType m_status;
  std::vector<char> m_buffer;
};

} // namespace hrd41

#endif //__ILRD_DRIVER_DATA_HPP__
