#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

#include "thread_pool.hpp"

using namespace hrd41;
using namespace std::chrono_literals;

// ── helpers ────────────────────────────────────────────────────────────────

class CountCommand : public ICommand
{
public:
    explicit CountCommand(std::atomic<int>& counter, ICommand::CMDPriority p = ICommand::Med)
        : ICommand(p), m_counter(counter)
    {}

    void Execute() override { ++m_counter; }

private:
    std::atomic<int>& m_counter;
};

class SleepCommand : public ICommand
{
public:
    explicit SleepCommand(std::chrono::milliseconds dur)
        : ICommand(ICommand::Med), m_dur(dur)
    {}

    void Execute() override { std::this_thread::sleep_for(m_dur); }

private:
    std::chrono::milliseconds m_dur;
};

static void pass(const char* name) { std::cout << "[PASS] " << name << '\n'; }
static void fail(const char* name) { std::cout << "[FAIL] " << name << '\n'; }
#define CHECK(name, cond) do { if (cond) pass(name); else fail(name); } while(0)

// ── tests ──────────────────────────────────────────────────────────────────

// Basic: commands execute and counter reaches expected value
void test_basic_execution()
{
    const int N = 20;
    std::atomic<int> counter{0};
    {
        ThreadPool tp(4);
        for (int i = 0; i < N; ++i)
        {
            std::shared_ptr<ICommand> cmd = std::make_shared<CountCommand>(counter);
            tp.AddCommand(cmd);
        }
    } // destructor calls Stop() – joins all threads

    CHECK("basic_execution", counter == N);
}

// GetSize returns the constructed size
void test_get_size()
{
    ThreadPool tp(6);
    CHECK("get_size", tp.GetSize() == 6);
}

// SetSize restarts pool with new thread count
void test_set_size()
{
    ThreadPool tp(2);
    tp.SetSize(5);
    CHECK("set_size", tp.GetSize() == 5);
}

// SetSize(0) throws std::length_error
void test_set_size_zero_throws()
{
    ThreadPool tp(2);
    bool threw = false;
    try { tp.SetSize(0); }
    catch (const std::length_error&) { threw = true; }
    CHECK("set_size_zero_throws", threw);
}

// AddCommand(nullptr) throws std::invalid_argument
void test_add_null_throws()
{
    ThreadPool tp(2);
    bool threw = false;
    try { tp.AddCommand(nullptr); }
    catch (const std::invalid_argument&) { threw = true; }
    CHECK("add_null_throws", threw);
}

// Suspend + Resume: commands added before Resume are all eventually executed
void test_suspend_resume()
{
    const int N = 10;
    std::atomic<int> counter{0};

    ThreadPool tp(4);
    tp.Suspend();

    for (int i = 0; i < N; ++i)
    {
        std::shared_ptr<ICommand> cmd = std::make_shared<CountCommand>(counter);
        tp.AddCommand(cmd);
    }

    // Commands should not run while suspended
    std::this_thread::sleep_for(50ms);
    int snap = counter.load();

    tp.Resume();
    std::this_thread::sleep_for(100ms);

    CHECK("suspend_blocks_execution",  snap == 0);
    CHECK("resume_allows_execution",   counter == N);
}

// Suspend followed immediately by Stop must not deadlock
void test_suspend_then_stop()
{
    ThreadPool tp(4);
    tp.Suspend();
    tp.Stop();  // Stop calls Resume() internally
    CHECK("suspend_then_stop_no_deadlock", true);
}

// Resume without prior Suspend is a no-op (no crash)
void test_resume_without_suspend()
{
    ThreadPool tp(2);
    tp.Resume();
    CHECK("resume_without_suspend", true);
}

// Priority: Admin commands execute before Low commands
void test_priority_ordering()
{
    // Single worker so ordering is deterministic
    ThreadPool tp(1);
    tp.Suspend(); // hold the worker

    std::atomic<int> order{0};

    // Use raw int captures via shared state to record which ran first
    std::atomic<int> low_seq{-1}, admin_seq{-1};

    class SeqCommand : public ICommand
    {
    public:
        SeqCommand(std::atomic<int>& seq, std::atomic<int>& order, CMDPriority p)
            : ICommand(p), m_seq(seq), m_order(order) {}
        void Execute() override { m_seq = m_order++; }
    private:
        std::atomic<int>& m_seq;
        std::atomic<int>& m_order;
    };

    std::shared_ptr<ICommand> low   = std::make_shared<SeqCommand>(low_seq,   order, ICommand::Low);
    std::shared_ptr<ICommand> admin = std::make_shared<SeqCommand>(admin_seq, order, ICommand::Admin);

    tp.AddCommand(low);   // pushed first
    tp.AddCommand(admin); // pushed second – but higher priority

    tp.Resume();
    std::this_thread::sleep_for(100ms);

    // Admin should have run before Low
    CHECK("priority_admin_before_low", admin_seq < low_seq);
}

// Multiple Suspend/Resume cycles work correctly
void test_multiple_suspend_resume_cycles()
{
    std::atomic<int> counter{0};
    ThreadPool tp(2);

    for (int cycle = 0; cycle < 3; ++cycle)
    {
        tp.Suspend();
        std::shared_ptr<ICommand> cmd = std::make_shared<CountCommand>(counter);
        tp.AddCommand(cmd);
        std::this_thread::sleep_for(30ms);
        tp.Resume();
        std::this_thread::sleep_for(50ms);
    }

    CHECK("multiple_suspend_resume_cycles", counter == 3);
}

// ── main ───────────────────────────────────────────────────────────────────

int main()
{
    test_basic_execution();
    test_get_size();
    test_set_size();
    test_set_size_zero_throws();
    test_add_null_throws();
    test_suspend_resume();
    test_suspend_then_stop();
    test_resume_without_suspend();
    test_priority_ordering();
    test_multiple_suspend_resume_cycles();

    std::cout << "\nDone.\n";
    return 0;
}
