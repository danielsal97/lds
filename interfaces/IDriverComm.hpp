// ============================================================================
// IDriverComm.hpp
// What:  Interface for driver communication protocols (NBD, iSCSI, etc)
// Why:   Abstracts protocol details so system can swap drivers without
//        changing storage layer logic
// How:   Implement ReceiveRequest() to get requests from kernel/client,
//        SendReplay() to send responses, Disconnect() for cleanup
// ============================================================================

#ifndef __ILRD_IDRIVER_COMM_HPP__
#define __ILRD_IDRIVER_COMM_HPP__

#include <memory> // shared_ptr

#include "DriverData.hpp"

namespace hrd41
{
class IDriverComm
{
public:
  IDriverComm() = default;
  IDriverComm(const IDriverComm& o_) = delete;
  IDriverComm& operator=(const IDriverComm& o_) = delete;
  virtual ~IDriverComm() = default;

  virtual std::shared_ptr<DriverData> ReceiveRequest() = 0;
  virtual void
  SendReplay(std::shared_ptr<DriverData> data_) = 0; // throw IOExcepton
  virtual void Disconnect() = 0;
  virtual int GetFD() = 0; // epoll use only
};

class DriverError : public std::runtime_error
{
public:
  DriverError(const std::string& msg_) : std::runtime_error(msg_) {}

  ~DriverError() = default;
};

} // namespace hrd41
#endif //__ILRD_IDRIVER_COMM_HPP__
