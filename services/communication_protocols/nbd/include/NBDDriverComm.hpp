// ============================================================================
// NBDDriverComm.hpp
// What:  Implements NBD (Network Block Device) protocol communication
// Why:   Allows the storage system to expose a virtual block device to the
//        kernel; kernel sends read/write/trim/flush commands, we process them
// How:   Creates socket pair with NBD kernel driver, receives requests via
//        ReceiveRequest(), processes them, sends responses via SendReplay()
// ============================================================================

#ifndef __ILRD_NBD_DRIVER_COMM_HPP__
#define __ILRD_NBD_DRIVER_COMM_HPP__

#include <thread> // std::thread

#include "DriverData.hpp"
#include "IDriverComm.hpp"

namespace hrd41
{

class NBDDriverComm : public IDriverComm
{
public:
  explicit NBDDriverComm(const std::string& deviceName_, size_t storage_size);

  explicit NBDDriverComm(const std::string& deviceName_, size_t block_size,
                         size_t num_blocks);

  NBDDriverComm(const NBDDriverComm& o_) = delete;
  NBDDriverComm& operator=(const NBDDriverComm& o_) = delete;

  ~NBDDriverComm() override;

  std::shared_ptr<DriverData> ReceiveRequest() override;       // DriverError
  void SendReplay(std::shared_ptr<DriverData> data_) override; // DriverError
  void Disconnect() override;                                  // DriverError

  int GetFD() override;

private:
  int m_nbdFd;
  int m_serverFd;
  int m_clientFd;

  std::thread m_listener;
  std::thread m_signal_thread;
  void ListenerThread();
  void SetUpSignals();
  void ReadAll(int fd, void* buf, size_t count);
  void WriteAll(int fd, const void* buf, size_t count);
};

class NBDDriverError : public DriverError
{
public:
  NBDDriverError(const std::string& msg_);
  ~NBDDriverError() = default;
};

} // namespace hrd41
#endif //__ILRD_NBD_DRIVER_COMM_HPP__
