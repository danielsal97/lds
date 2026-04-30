#include "thread_pool.hpp"
#include "ICommand.hpp"

#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace hrd41
{

std::mutex ThreadPool::m_mutex;
std::condition_variable ThreadPool::m_cv;

/*==================== Stop Exception ====================*/

namespace
{
struct StopException : public std::exception
{
    const char* what() const noexcept override { return "Stop command"; }
};
} // namespace

/*==================== Stop Command ====================*/

ThreadPool::StopCommand::StopCommand()
    : ICommand(Low)
{}

void ThreadPool::StopCommand::Execute()
{
    throw StopException();
}

/*==================== Suspend Command ====================*/

ThreadPool::SuspendCommand::SuspendCommand(ThreadPool* tp)
    : ICommand(High), m_tp(tp)
{}

void ThreadPool::SuspendCommand::Execute()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock, [this]{ return !m_tp->m_suspend_flag; });
}

/*==================== ThreadPool ====================*/

ThreadPool::ThreadPool(size_t ThreadsNum)
    : m_command(),
      m_threads(ThreadsNum),
      m_suspend_flag(false)
{
    InitThreads();
}

ThreadPool::~ThreadPool()
{
    Stop();
}

/*==================== Stop ====================*/

void ThreadPool::Stop()
{
    Resume(); // wake any suspended threads so they can receive stop commands
    std::shared_ptr<ICommand> stop_cmd = std::make_shared<StopCommand>();

    for (size_t i = 0; i < GetSize(); ++i)
    {
        m_command.Push(stop_cmd);
    }

    for (auto& thr : m_threads)
    {
        if (thr.joinable())
        {
            thr.join();
        }
    }
}

/*==================== Add Command ====================*/

void ThreadPool::AddCommand(std::shared_ptr<ICommand> cmd)
{
    if (!cmd)
    {
        throw std::invalid_argument("Null command");
    }

    m_command.Push(cmd);
}

/*==================== Set Size ====================*/

void ThreadPool::SetSize(size_t newSize)
{
    if (newSize == 0)
    {
        throw std::length_error("Size cannot be 0");
    }

    Stop();

    m_threads.clear();
    m_threads.resize(newSize);

    InitThreads();
}

size_t ThreadPool::GetSize() const
{
    return m_threads.size();
}

/*==================== Suspend / Resume ====================*/

void ThreadPool::Suspend()
{
    m_suspend_flag = true;
    std::shared_ptr<ICommand> suspend =
        std::make_shared<SuspendCommand>(this);

    for (size_t i = 0; i < m_threads.size(); ++i)
    {
        m_command.Push(suspend);
    }
}

void ThreadPool::Resume()
{
    m_suspend_flag = false;
    m_cv.notify_all();
}

/*==================== Force Stop ====================*/

#ifndef NDEBUG
void ThreadPool::ForceStop(std::chrono::milliseconds graceTime)
{
    std::this_thread::sleep_for(graceTime);
    std::terminate();
}
#endif

/*==================== Default Threads ====================*/

size_t ThreadPool::SetDefNumThreads()
{
    size_t ret = std::thread::hardware_concurrency();
    return ret > 0 ? ret : 1;
}

/*==================== Worker Thread ====================*/

void ThreadPool::ThreadFunc()
{
    while (true)
    {
        try
        {
            std::shared_ptr<ICommand> cmd = m_command.Pop();
            cmd->Execute();
        }
        catch (const StopException&)
        {
            break;
        }
        catch (...)
        {
            // Prevent worker thread from dying on user exception
        }
    }
}

/*==================== Init Threads ====================*/

void ThreadPool::InitThreads()
{
    for (auto& thr : m_threads)
    {
        thr = std::thread(&ThreadPool::ThreadFunc, this);
    }
}

} // namespace hrd41