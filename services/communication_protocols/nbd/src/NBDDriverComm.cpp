// ============================================================================
// NBDDriverComm.cpp
// What:  Implements NBD (Network Block Device) protocol communication
// Why:   Allows the storage system to expose a virtual block device to the
//        kernel; kernel sends read/write/trim/flush commands, we process them
// How:   Creates socket pair with NBD kernel driver, receives requests via
//        ReceiveRequest(), processes them, sends responses via SendReplay()
// ============================================================================

#include <arpa/inet.h>   // htonl, ntohl, htons
#include <cassert>       // assert
#include <cerrno>        // errno
#include <csignal>       // sigset_t, sigwait, pthread_sigmask, SIGINT, SIGTERM
#include <cstring>       // strerror, memcpy
#include <endian.h>      // be64toh
#include <fcntl.h>       // open, O_RDWR
#include <linux/nbd.h>   // nbd_request, nbd_reply, NBD_* constants
#include <memory>        // shared_ptr
#include <sys/ioctl.h>   // ioctl
#include <sys/socket.h>  // socketpair, AF_UNIX, SOCK_STREAM
#include <unistd.h>      // close, read, write

#include "NBDDriverComm.hpp"
#include "DriverData.hpp"

namespace hrd41
{

// -------------------- Helpers -------------------------

void NBDDriverComm::ReadAll(int fd, void* buf, size_t count)
{
  char* ptr = static_cast<char*>(buf);
  while (count > 0)
  {
    ssize_t n = ::read(fd, ptr, count);
    if (n <= 0)
    {
      throw NBDDriverError("ReadAll: failed to read from socket: " +
                           std::string(strerror(errno)));
    }
    ptr += n;
    count -= n;
  }
}

void NBDDriverComm::WriteAll(int fd, const void* buf, size_t count)
{
  const char* ptr = static_cast<const char*>(buf);
  while (count > 0)
  {
    ssize_t n = ::write(fd, ptr, count);
    if (n <= 0)
    {
      throw NBDDriverError("WriteAll: failed to write to socket: " +
                           std::string(strerror(errno)));
    }
    ptr += n;
    count -= n;
  }
}

// -------------------- ListenerThread --------------------

void NBDDriverComm::ListenerThread()
{
  sigset_t set;
  sigfillset(&set);
  pthread_sigmask(SIG_SETMASK, &set, nullptr);

  ioctl(m_nbdFd, NBD_SET_SOCK, m_clientFd);
  ioctl(m_nbdFd, NBD_DO_IT); // blocks here safely
  ioctl(m_nbdFd, NBD_CLEAR_QUE);
  ioctl(m_nbdFd, NBD_CLEAR_SOCK);
}

// -------------------- SetUpSignals --------------------

void NBDDriverComm::SetUpSignals()
{
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &set, nullptr);  // block in main thread

  m_signal_thread = std::thread([this, set]() mutable {
    int sig;
    sigwait(&set, &sig);
    Disconnect();
  });
}
// -------------------- Constructors --------------------

NBDDriverComm::NBDDriverComm(const std::string& dev_, size_t capacity_)
    : m_nbdFd(-1), m_serverFd(-1), m_clientFd(-1)
{
  int sp[2];
  int err = socketpair(AF_UNIX, SOCK_STREAM, 0, sp);
  assert(err == 0);

  m_serverFd = sp[0];
  m_clientFd = sp[1];

  m_nbdFd = open(dev_.c_str(), O_RDWR);
  if (-1 == m_nbdFd)
  {
    close(m_serverFd);
    close(m_clientFd);
    throw NBDDriverError("Failed to open " + dev_ + ": " + strerror(errno));
  }

  if (-1 == ioctl(m_nbdFd, NBD_SET_SIZE, capacity_))
  {
    close(m_nbdFd);
    close(m_serverFd);
    close(m_clientFd);
    throw NBDDriverError("NBD_SET_SIZE failed: " +
                         std::string(strerror(errno)));
  }

  if (-1 == ioctl(m_nbdFd, NBD_CLEAR_SOCK))
  {
    close(m_nbdFd);    // close fd
    close(m_serverFd); // close socket
    close(m_clientFd); // close socket
    throw NBDDriverError("NBD_CLEAR_SOCK failed: " +
                         std::string(strerror(errno)));
  }

  // set optional flags
  ioctl(m_nbdFd, NBD_SET_FLAGS, NBD_FLAG_SEND_TRIM | NBD_FLAG_SEND_FLUSH);

  SetUpSignals();

  if (-1 == ioctl(m_nbdFd, NBD_SET_SOCK, m_clientFd))
  {
    close(m_nbdFd);
    close(m_serverFd);
    close(m_clientFd);
    throw NBDDriverError("NBD_SET_SOCK failed: " +
                         std::string(strerror(errno)));
  }
  m_listener = std::thread(&NBDDriverComm::ListenerThread, this);
}

NBDDriverComm::NBDDriverComm(const std::string& dev_, size_t block_size_,
                             size_t num_blocks_)
    : NBDDriverComm(dev_, block_size_ * num_blocks_)
{
}

// -------------------- Destructor ----------------------

NBDDriverComm::~NBDDriverComm() { Disconnect(); }

// -------------------- ReceiveRequest ------------------

std::shared_ptr<DriverData> NBDDriverComm::ReceiveRequest()
{
  struct nbd_request request;
  ReadAll(m_serverFd, &request, sizeof(request));

  uint32_t magic = ntohl(request.magic);
  if (magic != NBD_REQUEST_MAGIC)
  {
    throw NBDDriverError("Invalid NBD request magic");
  }

  size_t type = ntohl(request.type);
  size_t from = be64toh(request.from);
  size_t len = ntohl(request.len);

  size_t handle = 0;
  std::memcpy(&handle, request.handle, sizeof(handle));

  DriverData::ActionType action;
  switch (type)
  {

  case NBD_CMD_READ:
    action = DriverData::READ;
    break;
  case NBD_CMD_WRITE:
    action = DriverData::WRITE;
    break;
  case NBD_CMD_DISC:
    action = DriverData::DISCONNECT;
    break;
  case NBD_CMD_FLUSH:
    action = DriverData::FLUSH;
    break;
  case NBD_CMD_TRIM:
    action = DriverData::TRIM;
    break;
  default:
    throw NBDDriverError("Unknown NBD command type: " + std::to_string(type));
  }

  std::shared_ptr<DriverData> ret(new DriverData(action, handle, from, len));

  if (action == DriverData::WRITE)
  {
    ReadAll(m_serverFd, ret->m_buffer.data(), len);
  }

  return ret;
}

// -------------------- SendReplay ----------------------

void NBDDriverComm::SendReplay(std::shared_ptr<DriverData> data_)
{
  struct nbd_reply reply;
  reply.magic = htonl(NBD_REPLY_MAGIC);
  reply.error = htonl(data_->m_status == DriverData::SUCCESS ? 0 : EIO);
  std::memcpy(reply.handle, &data_->m_handle, sizeof(reply.handle));

  WriteAll(m_serverFd, &reply, sizeof(reply));

  // For READ requests, send the data buffer after the reply header
  if (data_->m_type == DriverData::READ &&
      data_->m_status == DriverData::SUCCESS)
  {
    WriteAll(m_serverFd, data_->m_buffer.data(), data_->m_buffer.size());
  }
}

// -------------------- Disconnect ----------------------

void NBDDriverComm::Disconnect()
{
  if (m_nbdFd != -1)
  {
    ioctl(m_nbdFd, NBD_DISCONNECT);
    close(m_nbdFd);
    m_nbdFd = -1;
  }

  if (m_listener.joinable())
  {
    m_listener.join();
  }

  if (m_signal_thread.joinable())
  {
    m_signal_thread.join();
  }

  if (m_serverFd != -1)
  {
    close(m_serverFd);
    m_serverFd = -1;
  }
  if (m_clientFd != -1)
  {
    close(m_clientFd);
    m_clientFd = -1;
  }
}

// -------------------- GetFD ---------------------------

int NBDDriverComm::GetFD() { return m_serverFd; }

// -------------------- NBDDriverError ------------------

NBDDriverError::NBDDriverError(const std::string& msg_) : DriverError(msg_) {}

} // namespace hrd41
