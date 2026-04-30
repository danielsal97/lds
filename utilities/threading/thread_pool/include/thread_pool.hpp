// ============================================================================
// thread_pool.hpp
// What:  Worker thread pool that executes ICommand objects by priority
// Why:   Distributes work across multiple threads; WPQ orders by command
//        priority so High/Admin commands execute before Low/Med
// How:   AddCommand(cmd) pushes to priority queue; worker threads Pop() and
//        Execute(); Stop()/Suspend()/Resume() control operation
// ============================================================================

#ifndef _ILRD_TP_H__
#define _ILRD_TP_H__

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "ICommand.hpp"
#include "wpq.hpp"

namespace hrd41
{

class ThreadPool
{
public:
    explicit ThreadPool(size_t ThreadsNum = SetDefNumThreads());
    ThreadPool(const ThreadPool& o_) = delete;
    ThreadPool& operator=(const ThreadPool& o_) = delete;
    ~ThreadPool();

    void Stop();
    void AddCommand(std::shared_ptr<ICommand> cmd);

    void SetSize(size_t newSize);
    size_t GetSize() const;
    void Suspend();
    void Resume();

#ifndef NDEBUG
    void ForceStop(std::chrono::milliseconds graceTime);
#endif

private:
    struct Comparator
    {
        bool operator()(const std::shared_ptr<ICommand>& a,
                        const std::shared_ptr<ICommand>& b) const
        {
            return *a < *b;
        }
    };

    WPQ<std::shared_ptr<ICommand>, std::vector<std::shared_ptr<ICommand> >,
        Comparator> m_command;

    std::vector<std::thread> m_threads;

    static std::mutex m_mutex;
    static std::condition_variable m_cv;
    bool m_suspend_flag;

    void ThreadFunc();
    static size_t SetDefNumThreads();
    void InitThreads();

    class StopCommand : public ICommand
    {
    public:
        StopCommand();
        StopCommand(const StopCommand&) = delete;
        StopCommand& operator=(const StopCommand&) = delete;
        void Execute() override;
    };

    class SuspendCommand : public ICommand
    {
    public:
        explicit SuspendCommand(ThreadPool* tp);
        SuspendCommand(const SuspendCommand&) = delete;
        SuspendCommand& operator=(const SuspendCommand&) = delete;
        void Execute() override;
    private:
        ThreadPool* m_tp;
    };
};

} // namespace hrd41

#endif // _ILRD_TP_H__