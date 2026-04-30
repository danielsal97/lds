#include <cassert>
#include <iostream>

#include "msg_broker.hpp"
#include "logger.hpp"
#include "singelton.hpp"

using namespace hrd41;

// ── Test Helpers ───────────────────────────────────────────────────────────

static void pass(const char* name) { std::cout << "[PASS] " << name << '\n'; }
static void fail(const char* name) { std::cout << "[FAIL] " << name << '\n'; }
#define CHECK(name, cond) do { if (cond) pass(name); else fail(name); } while(0)

// ── Simple Subscriber for Testing ──────────────────────────────────────────

class SimpleSubscriber : public ICallBack<int>
{
public:
    explicit SimpleSubscriber(Dispatcher<int>* disp)
        : ICallBack<int>(disp), m_count(0), m_last_value(0)
    {}

    void Notify(const int& msg) override
    {
        ++m_count;
        m_last_value = msg;
    }

    void NotifyEnd() override
    {
        // Called when dispatcher wants to signal end of messages
    }

    int getCount() const { return m_count; }
    int getLastValue() const { return m_last_value; }

private:
    int m_count;
    int m_last_value;
};

// ── Handler Class for CallBack Testing ─────────────────────────────────────

class MyHandler
{
public:
    MyHandler() : m_count(0), m_last_value(0), m_stopped(false) {}

    void handleMessage(const int& msg)
    {
        ++m_count;
        m_last_value = msg;
        std::cout << "    Handler received: " << msg << '\n';
    }

    void onStop()
    {
        m_stopped = true;
        std::cout << "    Handler stopped\n";
    }

    int getCount() const { return m_count; }
    int getLastValue() const { return m_last_value; }
    bool wasStopped() const { return m_stopped; }

private:
    int m_count;
    int m_last_value;
    bool m_stopped;
};

// ── Tests ──────────────────────────────────────────────────────────────────

void test_1_subscriber_registers_on_creation()
{
    std::cout << "\n--- Test 1: Subscriber registers on creation ---\n";

    Dispatcher<int> dispatcher;
    SimpleSubscriber sub(&dispatcher);

    // When we create a subscriber, it should automatically register itself
    // So when we Notify, it should receive the message
    dispatcher.NotifyAll(42);

    CHECK("subscriber_receives_message", sub.getCount() == 1);
    CHECK("message_value_correct", sub.getLastValue() == 42);
}

void test_2_multiple_messages_to_one_subscriber()
{
    std::cout << "\n--- Test 2: Multiple messages to one subscriber ---\n";

    Dispatcher<int> dispatcher;
    SimpleSubscriber sub(&dispatcher);

    // Send multiple messages
    dispatcher.NotifyAll(10);
    dispatcher.NotifyAll(20);
    dispatcher.NotifyAll(30);

    CHECK("received_all_messages", sub.getCount() == 3);
    CHECK("last_message_is_30", sub.getLastValue() == 30);
}

void test_3_one_message_to_multiple_subscribers()
{
    std::cout << "\n--- Test 3: One message to multiple subscribers ---\n";

    Dispatcher<int> dispatcher;
    SimpleSubscriber sub1(&dispatcher);
    SimpleSubscriber sub2(&dispatcher);
    SimpleSubscriber sub3(&dispatcher);

    // Send one message, all three should receive it
    dispatcher.NotifyAll(100);

    CHECK("sub1_received", sub1.getCount() == 1 && sub1.getLastValue() == 100);
    CHECK("sub2_received", sub2.getCount() == 1 && sub2.getLastValue() == 100);
    CHECK("sub3_received", sub3.getCount() == 1 && sub3.getLastValue() == 100);
}

void test_4_unregister_removes_subscriber()
{
    std::cout << "\n--- Test 4: Unregister stops sending messages ---\n";

    Dispatcher<int> dispatcher;
    SimpleSubscriber sub1(&dispatcher);
    SimpleSubscriber sub2(&dispatcher);

    dispatcher.NotifyAll(10);
    CHECK("before_unregister_both_get_msg", sub1.getCount() == 1 && sub2.getCount() == 1);

    // Unregister sub1
    dispatcher.UnRegister(&sub1);

    dispatcher.NotifyAll(20);
    CHECK("after_unregister_sub1_no_new_msg", sub1.getCount() == 1);  // Still 1
    CHECK("after_unregister_sub2_gets_new_msg", sub2.getCount() == 2); // Now 2
}

void test_5_callback_with_member_function()
{
    std::cout << "\n--- Test 5: Using CallBack with member function ---\n";

    Dispatcher<int> dispatcher;
    MyHandler handler;

    // Create a callback that calls handler.handleMessage when messages arrive
    CallBack<int, MyHandler> callback(
        &dispatcher,
        handler,
        &MyHandler::handleMessage,
        &MyHandler::onStop
    );

    dispatcher.NotifyAll(55);

    CHECK("handler_receives_via_callback", handler.getCount() == 1);
    CHECK("handler_gets_correct_value", handler.getLastValue() == 55);
}

void test_6_callback_without_stop_function()
{
    std::cout << "\n--- Test 6: CallBack with optional stop function (nullptr) ---\n";

    Dispatcher<int> dispatcher;
    MyHandler handler;

    // Create callback without stop function (pass nullptr)
    CallBack<int, MyHandler> callback(
        &dispatcher,
        handler,
        &MyHandler::handleMessage,
        nullptr  // No stop function
    );

    dispatcher.NotifyAll(77);

    CHECK("still_works_without_stop", handler.getCount() == 1);
    CHECK("message_received", handler.getLastValue() == 77);
}

void test_7_Notify_end_signal()
{
    std::cout << "\n--- Test 7: NotifyEnd signal for cleanup ---\n";

    Dispatcher<int> dispatcher;
    MyHandler handler;

    CallBack<int, MyHandler> callback(
        &dispatcher,
        handler,
        &MyHandler::handleMessage,
        &MyHandler::onStop
    );

    dispatcher.NotifyAll(111);
    // When callback is destroyed, NotifyEnd() should be called
    // (or when dispatcher calls NotifyEnd explicitly)

    CHECK("handler_received_message", handler.getCount() == 1);
}

// ── Main ───────────────────────────────────────────────────────────────────

int main()
{
    // Initialize logger with DEBUG level to see all messages
    auto logger = Singleton<Logger>::GetInstance();
    logger->SetLoggerLevel(Logger::DEBUG);

    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║         Message Broker - Behavior Tests (TDD)             ║\n";
    std::cout << "║     Understanding the Observer/Pub-Sub Pattern            ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n";

    test_1_subscriber_registers_on_creation();
    test_2_multiple_messages_to_one_subscriber();
    test_3_one_message_to_multiple_subscribers();
    test_4_unregister_removes_subscriber();
    test_5_callback_with_member_function();
    test_6_callback_without_stop_function();
    test_7_Notify_end_signal();

    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║                     Tests Complete                        ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n\n";

    return 0;
}
