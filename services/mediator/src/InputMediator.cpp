#include "InputMediator.hpp"
#include "ICommand.hpp"
#include "IDriverComm.hpp"
#include "IStorage.hpp"
#include "DriverData.hpp"

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
    ThreadPool* pool)
    : m_driver(driver), m_storage(storage), m_pool(pool)
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
}

void InputMediator::Notify(int fd)
{
    (void)fd;

    auto request = m_driver->ReceiveRequest();
    
    auto cmd = std::make_shared<FunctionCommand>(

        [this, request]() {

            m_handlers.at(request->m_type)(request);

        }

    );

    m_pool->AddCommand(cmd);
}
} // namespace hrd41
