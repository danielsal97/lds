#include "InputMediator.hpp"
#include "IDriverComm.hpp"
#include "IStorage.hpp"
#include "DriverData.hpp"

namespace hrd41
{
    
InputMediator::InputMediator(IDriverComm* driver, IStorage* storage)
    : m_driver(driver), m_storage(storage)
{
    SetupHandlers();
}

void InputMediator::SetupHandlers()
{
    m_handlers[DriverData::READ] = [this](std::shared_ptr<DriverData> request) {
        m_storage->Read(request);
        m_driver->SendReplay(request);
    };

    m_handlers[DriverData::WRITE] = [this](std::shared_ptr<DriverData> request) {
        m_storage->Write(request);
        m_driver->SendReplay(request);
    };

    m_handlers[DriverData::FLUSH] = [this](std::shared_ptr<DriverData> request) {
        request->m_status = DriverData::SUCCESS;
        m_driver->SendReplay(request);
    };

    m_handlers[DriverData::TRIM] = [this](std::shared_ptr<DriverData> request) {
        request->m_status = DriverData::SUCCESS;
        m_driver->SendReplay(request);
    };

    m_handlers[DriverData::DISCONNECT] = [this](std::shared_ptr<DriverData> request) {
        (void)request;
    };
}

void InputMediator::Notify(int fd)
{
    (void)fd;
    auto request = m_driver->ReceiveRequest();
    m_handlers.at(request->m_type)(request);
}

} // namespace hrd41
