#include <iostream>
#include <cassert>
#include <cstring>
#include <iomanip>

#include "InputMediator.hpp"
#include "LocalStorage.hpp"
#include "IDriverComm.hpp"
#include "DriverData.hpp"
#include "logger.hpp"
#include "singelton.hpp"

using namespace hrd41;

class MockDriver : public IDriverComm
{
private:
    std::shared_ptr<DriverData> m_request;
    std::shared_ptr<DriverData> m_last_reply;

public:
    MockDriver() : m_request(nullptr), m_last_reply(nullptr) {}

    void SetRequest(std::shared_ptr<DriverData> req) { m_request = req; }
    std::shared_ptr<DriverData> GetLastReply() { return m_last_reply; }

    std::shared_ptr<DriverData> ReceiveRequest() override
    {
        auto logger = Singleton<Logger>::GetInstance();
        logger->Write("[MockDriver] ReceiveRequest() called", Logger::DEBUG);
        if (m_request)
        {
            std::string type_name;
            switch (m_request->m_type)
            {
            case DriverData::READ:
                type_name = "READ";
                break;
            case DriverData::WRITE:
                type_name = "WRITE";
                break;
            case DriverData::FLUSH:
                type_name = "FLUSH";
                break;
            case DriverData::TRIM:
                type_name = "TRIM";
                break;
            case DriverData::DISCONNECT:
                type_name = "DISCONNECT";
                break;
            default:
                type_name = "UNKNOWN";
            }
            logger->Write("Type: " + type_name + " | Offset: " + std::to_string(m_request->m_offset) +
                         " | Size: " + std::to_string(m_request->m_buffer.size()), Logger::DEBUG);
        }
        return m_request;
    }

    void SendReply(std::shared_ptr<DriverData> data_) override
    {
        m_last_reply = data_;
        auto logger = Singleton<Logger>::GetInstance();
        logger->Write("[MockDriver] SendReply() called", Logger::DEBUG);
        if (m_last_reply)
        {
            std::string status = (m_last_reply->m_status == DriverData::SUCCESS) ? "SUCCESS" : "FAILURE";
            std::string content_preview = "";
            if (m_last_reply->m_buffer.size() > 0 && m_last_reply->m_buffer[0] != '\0')
            {
                content_preview = " | Content: " + std::string(m_last_reply->m_buffer.begin(),
                                                          m_last_reply->m_buffer.begin() +
                                                          std::min(size_t(10), m_last_reply->m_buffer.size()));
            }
            logger->Write("Status: " + status + " | Buffer size: " + std::to_string(m_last_reply->m_buffer.size()) +
                         content_preview, Logger::DEBUG);
        }
    }

    void Disconnect() override {}
    int GetFD() override { return -1; }
};

int main()
{
    auto logger = Singleton<Logger>::GetInstance();
    logger->SetLoggerLevel(Logger::DEBUG);
    logger->SetFileName("test_input_mediator.log");

    logger->Write("==============================================", Logger::INFO);
    logger->Write("InputMediator Integration Tests", Logger::INFO);
    logger->Write("==============================================", Logger::INFO);

    try
    {
        logger->Write("[Setup] Creating LocalStorage (1024 bytes)...", Logger::INFO);
        LocalStorage storage(1024);
        logger->Write("✓ LocalStorage created", Logger::INFO);

        logger->Write("[Setup] Creating MockDriver...", Logger::INFO);
        MockDriver driver;
        logger->Write("✓ MockDriver created", Logger::INFO);

        logger->Write("[Setup] Creating InputMediator...", Logger::INFO);
        InputMediator mediator(&driver, &storage);
        logger->Write("✓ InputMediator created with handlers", Logger::INFO);

        logger->Write("----------------------------------------------", Logger::INFO);
        logger->Write("TEST 1: WRITE request", Logger::INFO);
        logger->Write("----------------------------------------------", Logger::INFO);
        {
            auto request = std::make_shared<DriverData>(
                DriverData::WRITE, 0, 0, 10);
            request->m_buffer.resize(10);
            std::strcpy(request->m_buffer.data(), "hello");

            logger->Write("[Test] WRITE request: handle=0, offset=0, size=10, data='hello'", Logger::INFO);
            driver.SetRequest(request);
            logger->Write("[Test] Calling mediator.Notify(0)...", Logger::INFO);
            mediator.Notify(0);

            auto reply = driver.GetLastReply();
            assert(reply != nullptr);
            assert(reply->m_status == DriverData::SUCCESS);
            logger->Write("[Result] ✓ WRITE completed successfully", Logger::INFO);
        }

        logger->Write("----------------------------------------------", Logger::INFO);
        logger->Write("TEST 2: READ request", Logger::INFO);
        logger->Write("----------------------------------------------", Logger::INFO);
        {
            auto request = std::make_shared<DriverData>(
                DriverData::READ, 1, 0, 10);
            request->m_buffer.resize(10);

            logger->Write("[Test] READ request: handle=1, offset=0, size=10", Logger::INFO);
            driver.SetRequest(request);
            logger->Write("[Test] Calling mediator.Notify(0)...", Logger::INFO);
            mediator.Notify(0);

            auto reply = driver.GetLastReply();
            assert(reply != nullptr);
            assert(reply->m_status == DriverData::SUCCESS);
            assert(std::strncmp(reply->m_buffer.data(), "hello", 5) == 0);
            std::string data_retrieved(reply->m_buffer.data(), reply->m_buffer.data() + 5);
            logger->Write("[Result] ✓ READ completed successfully | Data retrieved: '" + data_retrieved + "'", Logger::INFO);
        }

        logger->Write("----------------------------------------------", Logger::INFO);
        logger->Write("TEST 3: FLUSH request", Logger::INFO);
        logger->Write("----------------------------------------------", Logger::INFO);
        {
            auto request = std::make_shared<DriverData>(
                DriverData::FLUSH, 2, 0, 0);

            logger->Write("[Test] FLUSH request: handle=2", Logger::INFO);
            driver.SetRequest(request);
            logger->Write("[Test] Calling mediator.Notify(0)...", Logger::INFO);
            mediator.Notify(0);

            auto reply = driver.GetLastReply();
            assert(reply != nullptr);
            assert(reply->m_status == DriverData::SUCCESS);
            logger->Write("[Result] ✓ FLUSH completed successfully", Logger::INFO);
        }

        logger->Write("----------------------------------------------", Logger::INFO);
        logger->Write("TEST 4: TRIM request", Logger::INFO);
        logger->Write("----------------------------------------------", Logger::INFO);
        {
            auto request = std::make_shared<DriverData>(
                DriverData::TRIM, 3, 0, 0);

            logger->Write("[Test] TRIM request: handle=3", Logger::INFO);
            driver.SetRequest(request);
            logger->Write("[Test] Calling mediator.Notify(0)...", Logger::INFO);
            mediator.Notify(0);

            auto reply = driver.GetLastReply();
            assert(reply != nullptr);
            assert(reply->m_status == DriverData::SUCCESS);
            logger->Write("[Result] ✓ TRIM completed successfully", Logger::INFO);
        }

        logger->Write("----------------------------------------------", Logger::INFO);
        logger->Write("TEST 5: Multiple WRITE/READ cycle", Logger::INFO);
        logger->Write("----------------------------------------------", Logger::INFO);
        {
            logger->Write("[Test] Writing 'test data' at offset=100", Logger::INFO);
            auto write_req = std::make_shared<DriverData>(
                DriverData::WRITE, 4, 100, 20);
            write_req->m_buffer.resize(20);
            std::strcpy(write_req->m_buffer.data(), "test data");

            driver.SetRequest(write_req);
            mediator.Notify(0);

            logger->Write("[Test] Reading back from offset=100...", Logger::INFO);
            auto read_req = std::make_shared<DriverData>(
                DriverData::READ, 5, 100, 20);
            read_req->m_buffer.resize(20);

            driver.SetRequest(read_req);
            mediator.Notify(0);

            auto reply = driver.GetLastReply();
            assert(reply != nullptr);
            assert(std::strncmp(reply->m_buffer.data(), "test data", 9) == 0);
            std::string data_verified(reply->m_buffer.data(), reply->m_buffer.data() + 9);
            logger->Write("[Result] ✓ Multiple WRITE/READ cycle works | Data verified: '" + data_verified + "'", Logger::INFO);
        }

        logger->Write("==============================================", Logger::INFO);
        logger->Write("✅ All InputMediator tests PASSED!", Logger::INFO);
        logger->Write("==============================================", Logger::INFO);
    }
    catch (const std::exception& e)
    {
        auto logger = Singleton<Logger>::GetInstance();
        logger->Write("❌ Test FAILED: " + std::string(e.what()), Logger::ERROR);
        return 1;
    }

    return 0;
}
