// ============================================================================
// TCPDriverComm.hpp
// What:  TCP server implementation of IDriverComm for multi-client support
// Why:   Allows multiple TCP clients to send block device commands;
//        supports reconnection by IP with persistent state tracking
// How:   Keeps m_listen_fd open forever, Reactor adds/removes client FDs
//        dynamically. Each ReceiveRequest(fd) reads from that client.
//        SendReply uses data->m_source_fd to route replies.
// ============================================================================

#ifndef __ILRD_TCP_DRIVER_COMM_HPP__
#define __ILRD_TCP_DRIVER_COMM_HPP__

#include <memory>
#include <stdexcept>
#include <map>
#include <set>
#include <vector>
#include <mutex>
#include <string>

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

    std::shared_ptr<DriverData> ReceiveRequest(int fd) override;
    void SendReply(std::shared_ptr<DriverData> data_) override;
    void Disconnect() override;
    int GetFD() override;

    void AddClientFD(int fd) override { }     // TCP: called by Reactor (no-op, internal accept)
    void RemoveClientFD(int fd) override;     // TCP: called by Reactor on client disconnect
    int TryAccept(int fd) override;           // If fd==listen_fd, accept new client and return fd; else -1

    // Called by Reactor when m_listen_fd is ready (new connection)
    int AcceptNextClient();

private:
    int m_listen_fd;                         // Listen socket (permanent)
    std::set<int> m_client_fds;              // All connected client FDs
    mutable std::mutex m_clients_lock;

    // Per-client state: IP → (offset → size written)
    std::map<std::string, std::map<size_t, size_t>> m_client_writes;
    mutable std::mutex m_writes_lock;

    // Map client FD to IP for reconnection tracking
    std::map<int, std::string> m_fd_to_ip;
    mutable std::mutex m_fd_to_ip_lock;

    // Allocation tracking (TCP-only)
    std::map<size_t, std::vector<char>> m_allocations;
    mutable std::mutex m_alloc_lock;

    void ReadAll(int fd, void* buf, size_t count);
    void WriteAll(int fd, const void* buf, size_t count);
    void UpdateAllocation(size_t offset, const std::vector<char>& data);
    std::vector<char> GetAllocation(size_t offset) const;

    std::string GetClientIP(int fd) const;
    void RecordClientIP(int fd, const std::string& ip);
    void ForgetClientIP(int fd);

    void SaveAllocations();  // Persist allocations to disk
    void LoadAllocations();  // Load allocations from disk on startup
};

class TCPDriverError : public DriverError
{
public:
    explicit TCPDriverError(const std::string& msg_);
    ~TCPDriverError() = default;
};

} // namespace hrd41

#endif // __ILRD_TCP_DRIVER_COMM_HPP__
