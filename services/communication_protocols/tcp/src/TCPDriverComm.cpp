#include "TCPDriverComm.hpp"

#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#include <endian.h>

namespace hrd41
{

TCPDriverError::TCPDriverError(const std::string& msg_)
    : DriverError(msg_)
{
}

TCPDriverComm::TCPDriverComm(int port_)
    : m_listen_fd(-1), m_client_fd(-1)
{
    m_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listen_fd < 0)
    {
        throw TCPDriverError("Failed to create listen socket: " + std::string(strerror(errno)));
    }

    int reuse = 1;
    if (setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
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
        close(m_listen_fd);
        throw TCPDriverError("Failed to bind to port " + std::to_string(port_) + ": " + std::string(strerror(errno)));
    }

    if (listen(m_listen_fd, 1) < 0)
    {
        close(m_listen_fd);
        throw TCPDriverError("Failed to listen: " + std::string(strerror(errno)));
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    m_client_fd = accept(m_listen_fd, (struct sockaddr*)&client_addr, &client_addr_len);
    if (m_client_fd < 0)
    {
        close(m_listen_fd);
        throw TCPDriverError("Failed to accept connection: " + std::string(strerror(errno)));
    }

    close(m_listen_fd);
    m_listen_fd = -1;
}

TCPDriverComm::~TCPDriverComm()
{
    if (m_client_fd >= 0)
    {
        close(m_client_fd);
        m_client_fd = -1;
    }
    if (m_listen_fd >= 0)
    {
        close(m_listen_fd);
        m_listen_fd = -1;
    }
}

void TCPDriverComm::ReadAll(int fd, void* buf, size_t count)
{
    char* ptr = static_cast<char*>(buf);
    size_t remaining = count;
    while (remaining > 0)
    {
        ssize_t n = read(fd, ptr, remaining);
        if (n <= 0)
        {
            throw TCPDriverError("Read failed: " + std::string(strerror(errno)));
        }
        ptr += n;
        remaining -= n;
    }
}

void TCPDriverComm::WriteAll(int fd, const void* buf, size_t count)
{
    const char* ptr = static_cast<const char*>(buf);
    size_t remaining = count;
    while (remaining > 0)
    {
        ssize_t n = write(fd, ptr, remaining);
        if (n <= 0)
        {
            throw TCPDriverError("Write failed: " + std::string(strerror(errno)));
        }
        ptr += n;
        remaining -= n;
    }
}

std::shared_ptr<DriverData> TCPDriverComm::ReceiveRequest()
{
    struct RequestHeader
    {
        uint32_t type;
        uint64_t handle;
        uint64_t offset;
        uint32_t len;
    } __attribute__((packed));

    RequestHeader header;
    ReadAll(m_client_fd, &header, sizeof(header));

    uint32_t type = ntohl(header.type);
    uint64_t handle = be64toh(header.handle);
    uint64_t offset = be64toh(header.offset);
    uint32_t len = ntohl(header.len);

    auto ret = std::make_shared<DriverData>(
        static_cast<DriverData::ActionType>(type),
        handle,
        offset,
        len
    );

    if (type == DriverData::WRITE)
    {
        ReadAll(m_client_fd, ret->m_buffer.data(), len);
        UpdateAllocation(offset, ret->m_buffer);  // Track allocation in TCP layer
    }

    return ret;
}

void TCPDriverComm::SendReply(std::shared_ptr<DriverData> data_)
{
    struct ReplyHeader
    {
        uint32_t error;
        uint64_t handle;
        uint32_t len;
    } __attribute__((packed));

    ReplyHeader header;
    header.error = htonl(data_->m_status == DriverData::SUCCESS ? 0 : EIO);
    header.handle = htobe64(data_->m_handle);

    if (data_->m_type == DriverData::GET_SIZE)
    {
        auto alloc = GetAllocation(data_->m_offset);
        header.len = htonl(alloc.size());
    }
    else
    {
        header.len = htonl(0);
    }

    WriteAll(m_client_fd, &header, sizeof(header));

    if (data_->m_type == DriverData::READ && data_->m_status == DriverData::SUCCESS)
    {
        auto alloc = GetAllocation(data_->m_offset);
        if (!alloc.empty())
        {
            WriteAll(m_client_fd, alloc.data(), alloc.size());
        }
    }
}

void TCPDriverComm::Disconnect()
{
    if (m_client_fd >= 0)
    {
        close(m_client_fd);
        m_client_fd = -1;
    }
}

int TCPDriverComm::GetFD()
{
    return m_client_fd;
}

void TCPDriverComm::UpdateAllocation(size_t offset, const std::vector<char>& data)
{
    std::lock_guard<std::mutex> lock(m_alloc_lock);
    m_allocations[offset] = data;
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

} // namespace hrd41
