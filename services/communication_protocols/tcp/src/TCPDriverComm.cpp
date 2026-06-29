#include <cstring>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#include <endian.h>

#include "TCPDriverComm.hpp"
#include "logger.hpp"

namespace hrd41
{

TCPDriverError::TCPDriverError(const std::string& msg_)
    : DriverError(msg_)
{
}

TCPDriverComm::TCPDriverComm(int port_)
    : m_listen_fd(-1)
{
    auto logger = Singleton<Logger>::GetInstance();
    logger->Write("TCPDriverComm: Initializing on port " + std::to_string(port_), Logger::INFO);

    m_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listen_fd < 0)
    {
        logger->Write("Failed to create listen socket: " + std::string(strerror(errno)), Logger::ERROR);
        throw TCPDriverError("Failed to create listen socket: " + std::string(strerror(errno)));
    }

    int reuse = 1;
    if (setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        logger->Write("Failed to set SO_REUSEADDR: " + std::string(strerror(errno)), Logger::ERROR);
        close(m_listen_fd);
        throw TCPDriverError("Failed to set SO_REUSEADDR: " + std::string(strerror(errno)));
    }

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);

    if (bind(m_listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        logger->Write("Failed to bind to port " + std::to_string(port_) + ": " + std::string(strerror(errno)), Logger::ERROR);
        close(m_listen_fd);
        throw TCPDriverError("Failed to bind to port " + std::to_string(port_) + ": " + std::string(strerror(errno)));
    }
    logger->Write("Successfully bound to port " + std::to_string(port_), Logger::DEBUG);

    if (listen(m_listen_fd, 10) < 0)
    {
        logger->Write("Failed to listen: " + std::string(strerror(errno)), Logger::ERROR);
        close(m_listen_fd);
        throw TCPDriverError("Failed to listen: " + std::string(strerror(errno)));
    }
    logger->Write("Listening for client connections on port " + std::to_string(port_), Logger::INFO);

    // Load persistent allocations from previous sessions
    LoadAllocations();
}

TCPDriverComm::~TCPDriverComm()
{
    auto logger = Singleton<Logger>::GetInstance();

    // Close all client connections
    {
        std::lock_guard<std::mutex> lock(m_clients_lock);
        for (int fd : m_client_fds)
        {
            logger->Write("Closing client fd " + std::to_string(fd), Logger::DEBUG);
            close(fd);
        }
        m_client_fds.clear();
    }

    // Close listen socket
    if (m_listen_fd >= 0)
    {
        logger->Write("Closing listen socket", Logger::DEBUG);
        close(m_listen_fd);
        m_listen_fd = -1;
    }
    logger->Write("TCPDriverComm destroyed", Logger::INFO);
}

int TCPDriverComm::GetFD()
{
    // Always return LISTEN_FD - Reactor monitors this
    // When new connection arrives, Reactor calls AcceptNextClient()
    // When client data arrives, Reactor passes fd to ReceiveRequest()
    return m_listen_fd;
}

int TCPDriverComm::AcceptNextClient()
{
    auto logger = Singleton<Logger>::GetInstance();

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int new_fd = accept(m_listen_fd, (struct sockaddr*)&client_addr, &client_addr_len);
    if (new_fd < 0)
    {
        logger->Write("Failed to accept connection: " + std::string(strerror(errno)), Logger::ERROR);
        return -1;
    }

    std::string client_ip = inet_ntoa(client_addr.sin_addr);

    {
        std::lock_guard<std::mutex> lock(m_clients_lock);
        m_client_fds.insert(new_fd);
    }

    RecordClientIP(new_fd, client_ip);

    logger->Write("Client connected from " + client_ip + " (fd=" + std::to_string(new_fd) + ")", Logger::INFO);

    return new_fd;
}

void TCPDriverComm::RemoveClientFD(int fd)
{
    auto logger = Singleton<Logger>::GetInstance();

    {
        std::lock_guard<std::mutex> lock(m_clients_lock);
        m_client_fds.erase(fd);
    }

    std::string ip = GetClientIP(fd);
    ForgetClientIP(fd);

    close(fd);
    logger->Write("Client disconnected from " + ip + " (fd=" + std::to_string(fd) + ")", Logger::INFO);
}

void TCPDriverComm::ReadAll(int fd, void* buf, size_t count)
{
    auto logger = Singleton<Logger>::GetInstance();
    char* ptr = static_cast<char*>(buf);
    size_t remaining = count;

    while (remaining > 0)
    {
        ssize_t n = read(fd, ptr, remaining);
        if (n < 0)
        {
            logger->Write("Read failed: " + std::string(strerror(errno)), Logger::ERROR);
            throw TCPDriverError("Read failed: " + std::string(strerror(errno)));
        }
        if (n == 0)
        {
            logger->Write("Client disconnected (EOF on read)", Logger::INFO);
            throw TCPDriverError("Client disconnected");
        }
        ptr += n;
        remaining -= n;
    }
}

void TCPDriverComm::WriteAll(int fd, const void* buf, size_t count)
{
    auto logger = Singleton<Logger>::GetInstance();
    const char* ptr = static_cast<const char*>(buf);
    size_t remaining = count;

    while (remaining > 0)
    {
        ssize_t n = write(fd, ptr, remaining);
        if (n < 0)
        {
            logger->Write("Write failed: " + std::string(strerror(errno)), Logger::ERROR);
            throw TCPDriverError("Write failed: " + std::string(strerror(errno)));
        }
        if (n == 0)
        {
            logger->Write("Write returned 0 (client likely disconnected)", Logger::INFO);
            throw TCPDriverError("Write failed - client disconnected");
        }
        ptr += n;
        remaining -= n;
    }
}

std::shared_ptr<DriverData> TCPDriverComm::ReceiveRequest(int fd)
{
    auto logger = Singleton<Logger>::GetInstance();

    struct RequestHeader
    {
        uint32_t type;
        uint64_t handle;
        uint64_t offset;
        uint32_t len;
    } __attribute__((packed));

    RequestHeader header;
    logger->Write("ReceiveRequest: Reading from fd=" + std::to_string(fd), Logger::DEBUG);

    ReadAll(fd, &header, sizeof(header));

    uint32_t type = ntohl(header.type);
    uint64_t handle = be64toh(header.handle);
    uint64_t offset = be64toh(header.offset);
    uint32_t len = ntohl(header.len);

    const char* typeStr = (type == 0) ? "READ" : (type == 1) ? "WRITE" : (type == 2) ? "FLUSH" :
                          (type == 3) ? "TRIM" : (type == 4) ? "CYCLE" : "GET_SIZE";
    logger->Write("Received request: type=" + std::string(typeStr) + " handle=" + std::to_string(handle) +
                  " offset=" + std::to_string(offset) + " len=" + std::to_string(len), Logger::DEBUG);

    auto ret = std::make_shared<DriverData>(
        static_cast<DriverData::ActionType>(type),
        handle,
        offset,
        len
    );

    ret->m_source_fd = fd;  // Store source FD for reply routing

    if (type == DriverData::WRITE)
    {
        ReadAll(fd, ret->m_buffer.data(), len);
        UpdateAllocation(offset, ret->m_buffer);
        SaveAllocations();  // Persist to disk after each write
        logger->Write("[WRITE] Received " + std::to_string(len) + " bytes at offset " +
                      std::to_string(offset) + " (handle=" + std::to_string(handle) + ")", Logger::INFO);
    }
    else if (type == DriverData::READ)
    {
        logger->Write("[READ] Requested " + std::to_string(len) + " bytes from offset " +
                      std::to_string(offset) + " (handle=" + std::to_string(handle) + ")", Logger::INFO);
    }

    return ret;
}

void TCPDriverComm::SendReply(std::shared_ptr<DriverData> data_)
{
    auto logger = Singleton<Logger>::GetInstance();

    if (data_->m_source_fd < 0)
    {
        logger->Write("SendReply: Invalid source FD", Logger::ERROR);
        return;
    }

    struct ReplyHeader
    {
        uint32_t error;
        uint64_t handle;
        uint32_t len;
    } __attribute__((packed));

    // Compute reply length and whether to send payload based on request type
    uint32_t reply_len = 0;
    bool send_payload = false;

    if (data_->m_status == DriverData::SUCCESS)
    {
        switch (data_->m_type)
        {
            case DriverData::READ:
            case DriverData::LIST_OFFSETS:
                reply_len = data_->m_buffer.size();
                send_payload = !data_->m_buffer.empty();
                break;

            case DriverData::GET_SIZE:
                reply_len = data_->m_len;  // Size is in header, not payload
                send_payload = false;
                break;

            case DriverData::WRITE:
            case DriverData::FLUSH:
            case DriverData::TRIM:
            case DriverData::DISCONNECT:
            default:
                reply_len = 0;
                send_payload = false;
                break;
        }
    }

    ReplyHeader header;
    header.error = htonl(data_->m_status == DriverData::SUCCESS ? 0 : EIO);
    header.handle = htobe64(data_->m_handle);
    header.len = htonl(reply_len);

    const char* statusStr = (data_->m_status == DriverData::SUCCESS) ? "SUCCESS" : "ERROR";
    logger->Write("Sending reply: handle=" + std::to_string(data_->m_handle) + " status=" + statusStr +
                  " len=" + std::to_string(reply_len) + " to fd=" + std::to_string(data_->m_source_fd), Logger::DEBUG);

    try
    {
        WriteAll(data_->m_source_fd, &header, sizeof(header));

        if (send_payload)
        {
            WriteAll(data_->m_source_fd, data_->m_buffer.data(), data_->m_buffer.size());

            if (data_->m_type == DriverData::READ)
            {
                logger->Write("[READ] Sent " + std::to_string(data_->m_buffer.size()) + " bytes from offset " +
                              std::to_string(data_->m_offset) + " (handle=" + std::to_string(data_->m_handle) + ")", Logger::INFO);
            }
            else if (data_->m_type == DriverData::LIST_OFFSETS)
            {
                logger->Write("[LIST_OFFSETS] Sent " + std::to_string(data_->m_buffer.size()) + " bytes of offset list", Logger::INFO);
            }
        }
        else if (data_->m_type == DriverData::GET_SIZE && data_->m_status == DriverData::SUCCESS)
        {
            logger->Write("[GET_SIZE] Offset " + std::to_string(data_->m_offset) + " has " + std::to_string(data_->m_len) +
                          " bytes (handle=" + std::to_string(data_->m_handle) + ")", Logger::INFO);
        }
        else if (data_->m_type == DriverData::WRITE && data_->m_status == DriverData::SUCCESS)
        {
            logger->Write("[WRITE] Confirmed write at offset " + std::to_string(data_->m_offset) +
                          " (handle=" + std::to_string(data_->m_handle) + ")", Logger::INFO);
        }
    }
    catch (const std::exception& e)
    {
        logger->Write("Failed to send reply: " + std::string(e.what()), Logger::ERROR);
        // Client disconnected - will be cleaned up by Reactor
    }
}

void TCPDriverComm::Disconnect()
{
    auto logger = Singleton<Logger>::GetInstance();
    logger->Write("Disconnect called", Logger::DEBUG);
    // Reactor will handle actual cleanup via RemoveClientFD
}

void TCPDriverComm::UpdateAllocation(size_t offset, const std::vector<char>& data)
{
    std::lock_guard<std::mutex> lock(m_alloc_lock);
    // Append to existing data at offset (don't replace)
    m_allocations[offset].insert(m_allocations[offset].end(),
                                  data.begin(), data.end());
}

std::vector<char> TCPDriverComm::GetAllocation(size_t offset) const
{
    std::lock_guard<std::mutex> lock(m_alloc_lock);
    auto it = m_allocations.find(offset);
    if (it != m_allocations.end())
    {
        return it->second;
    }
    return std::vector<char>();
}

std::string TCPDriverComm::GetClientIP(int fd) const
{
    std::lock_guard<std::mutex> lock(m_fd_to_ip_lock);
    auto it = m_fd_to_ip.find(fd);
    if (it != m_fd_to_ip.end())
    {
        return it->second;
    }
    return "";
}

void TCPDriverComm::RecordClientIP(int fd, const std::string& ip)
{
    std::lock_guard<std::mutex> lock(m_fd_to_ip_lock);
    m_fd_to_ip[fd] = ip;
}

void TCPDriverComm::ForgetClientIP(int fd)
{
    std::lock_guard<std::mutex> lock(m_fd_to_ip_lock);
    m_fd_to_ip.erase(fd);
}

int TCPDriverComm::TryAccept(int fd)
{
    if (fd != m_listen_fd)
    {
        return -1;  // Not the listen fd, let mediator handle as normal request
    }

    // New connection ready on listen socket, accept it
    return AcceptNextClient();
}

void TCPDriverComm::SaveAllocations()
{
    auto logger = Singleton<Logger>::GetInstance();
    std::lock_guard<std::mutex> lock(m_alloc_lock);

    std::ofstream file(".tcp_allocations", std::ios::binary);
    if (!file.is_open())
    {
        logger->Write("Failed to open allocations file for writing", Logger::ERROR);
        return;
    }

    for (const auto& [offset, data] : m_allocations)
    {
        uint64_t off = htobe64(offset);
        uint64_t size = htobe64(data.size());
        file.write(reinterpret_cast<const char*>(&off), sizeof(off));
        file.write(reinterpret_cast<const char*>(&size), sizeof(size));
        file.write(data.data(), data.size());
    }

    file.close();
    logger->Write("Allocations saved to disk (" + std::to_string(m_allocations.size()) + " offsets)", Logger::DEBUG);
}

void TCPDriverComm::LoadAllocations()
{
    auto logger = Singleton<Logger>::GetInstance();
    std::lock_guard<std::mutex> lock(m_alloc_lock);

    std::ifstream file(".tcp_allocations", std::ios::binary);
    if (!file.is_open())
    {
        logger->Write("No saved allocations found (first run)", Logger::INFO);
        return;
    }

    uint64_t count = 0;
    while (file.good())
    {
        uint64_t off_bytes, size_bytes;
        file.read(reinterpret_cast<char*>(&off_bytes), sizeof(off_bytes));
        if (!file.good()) break;

        file.read(reinterpret_cast<char*>(&size_bytes), sizeof(size_bytes));
        if (!file.good()) break;

        uint64_t offset = be64toh(off_bytes);
        uint64_t size = be64toh(size_bytes);

        std::vector<char> data(size);
        file.read(data.data(), size);
        if (!file.good() && !file.eof()) break;

        m_allocations[offset] = data;
        count++;
    }

    file.close();
    logger->Write("Loaded " + std::to_string(count) + " offsets from disk", Logger::INFO);
}

} // namespace hrd41
