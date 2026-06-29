#include "InputMediator.hpp"
#include "ICommand.hpp"
#include "IDriverComm.hpp"
#include "IStorage.hpp"
#include "DriverData.hpp"
#include <endian.h>

namespace hrd41
{
 class FunctionCommand : public ICommand
{
public:
    explicit FunctionCommand(std::function<void()> func)
        : m_func(std::move(func))
    {}

    void Execute() override
    {
        m_func();
    }

private:
    std::function<void()> m_func;
};   



InputMediator::InputMediator(
    IDriverComm* driver,
    IStorage* storage,
    ThreadPool* pool,
    Reactor* reactor)
    : m_driver(driver), m_storage(storage), m_pool(pool), m_reactor(reactor)
{
    SetupHandlers();
}

void InputMediator::SetupHandlers()
{
    m_handlers[DriverData::READ] = [this](std::shared_ptr<DriverData> request) {
        m_storage->Read(request);
        m_driver->SendReply(request);
    };

    m_handlers[DriverData::WRITE] = [this](std::shared_ptr<DriverData> request) {
        m_storage->Write(request);
        m_driver->SendReply(request);
    };

    m_handlers[DriverData::FLUSH] = [this](std::shared_ptr<DriverData> request) {
        request->m_status = DriverData::SUCCESS;
        m_driver->SendReply(request);
    };

    m_handlers[DriverData::TRIM] = [this](std::shared_ptr<DriverData> request) {
        request->m_status = DriverData::SUCCESS;
        m_driver->SendReply(request);
    };

    m_handlers[DriverData::DISCONNECT] = [this](std::shared_ptr<DriverData> request) {
        (void)request;
    };

    m_handlers[DriverData::GET_SIZE] = [this](std::shared_ptr<DriverData> request) {
        request->m_len = m_storage->GetDataSize(request->m_offset);
        request->m_status = DriverData::SUCCESS;
        m_driver->SendReply(request);
    };

    m_handlers[DriverData::LIST_OFFSETS] = [this](std::shared_ptr<DriverData> request) {
        auto offsets = m_storage->ListOffsets();
        request->m_buffer.clear();
        for (const auto& [offset, size] : offsets)
        {
            uint64_t net_offset = htobe64(offset);
            uint64_t net_size = htobe64(size);
            char* off_ptr = reinterpret_cast<char*>(&net_offset);
            char* size_ptr = reinterpret_cast<char*>(&net_size);
            request->m_buffer.insert(
                request->m_buffer.end(),
                off_ptr,
                off_ptr + sizeof(net_offset)
            );
            request->m_buffer.insert(
                request->m_buffer.end(),
                size_ptr,
                size_ptr + sizeof(net_size)
            );
        }
        request->m_len = request->m_buffer.size();
        request->m_status = DriverData::SUCCESS;
        m_driver->SendReply(request);
    };
}

void InputMediator::Notify(int fd)
{
    // Check if this is a new connection event (listen_fd ready)
    int new_client_fd = m_driver->TryAccept(fd);
    if (new_client_fd >= 0)
    {
        // New TCP client connected, add it to Reactor for future events
        m_reactor->Add(new_client_fd);
        return;
    }

    // Regular client request - fd is a client socket ready for data
    try
    {
        auto request = m_driver->ReceiveRequest(fd);

        auto cmd = std::make_shared<FunctionCommand>(
            [this, request]() {
                m_handlers.at(request->m_type)(request);
            }
        );

        m_pool->AddCommand(cmd);
    }
    catch (const std::exception& e)
    {
        // Client disconnected or read failed - clean up
        m_reactor->Remove(fd);
        m_driver->RemoveClientFD(fd);
    }
}
} // namespace hrd41
