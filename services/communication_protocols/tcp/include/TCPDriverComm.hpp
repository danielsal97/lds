// ============================================================================
// TCPDriverComm.hpp
// What:  TCP server implementation of IDriverComm protocol
// Why:   Allows remote clients to send block device commands (READ/WRITE/etc)
//        over TCP instead of kernel NBD; drop-in replacement for NBDDriverComm
// How:   Listen on port, accept connection, read/write custom binary frames
//        matching DriverData structure (type|handle|offset|len) in big-endian
// ============================================================================

#ifndef __ILRD_TCP_DRIVER_COMM_HPP__
#define __ILRD_TCP_DRIVER_COMM_HPP__

#include <memory>
#include <stdexcept>
#include <map>
#include <vector>
#include <mutex>

#include "IDriverComm.hpp"
#include "DriverData.hpp"

namespace hrd41
{

class TCPDriverComm : public IDriverComm
{
public:
    explicit TCPDriverComm(int port_);

    TCPDriverComm(const TCPDriverComm& o_) = delete;
    TCPDriverComm& operator=(const TCPDriverComm& o_) = delete;

    ~TCPDriverComm() override;

    std::shared_ptr<DriverData> ReceiveRequest() override;
    void SendReplay(std::shared_ptr<DriverData> data_) override;
    void Disconnect() override;
    int GetFD() override;

private:
    int m_listen_fd;
    int m_client_fd;

    std::map<size_t, std::vector<char>> m_allocations;  // TCP-only allocation tracking
    mutable std::mutex m_alloc_lock;

    void ReadAll(int fd, void* buf, size_t count);
    void WriteAll(int fd, const void* buf, size_t count);
    void UpdateAllocation(size_t offset, const std::vector<char>& data);
    std::vector<char> GetAllocation(size_t offset) const;
};

class TCPDriverError : public DriverError
{
public:
    explicit TCPDriverError(const std::string& msg_);
    ~TCPDriverError() = default;
};

} // namespace hrd41

#endif // __ILRD_TCP_DRIVER_COMM_HPP__
