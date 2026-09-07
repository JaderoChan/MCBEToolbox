#include "thread_pool.hpp"

ThreadPool::ThreadPool(std::size_t numThreads)
    : stop_(false), unfinished_(0)
{
    for (std::size_t i = 0; i < numThreads; ++i)
        workers_.emplace_back(&ThreadPool::work, this);
}

ThreadPool::~ThreadPool()
{
    shutdown();
}

void ThreadPool::work()
{
    while (true)
    {
        std::function<void ()> task;
        {
            std::unique_lock<std::mutex> locker(mtx_);
            stopOrNotEmptyCv_.wait(locker, [this]() { return stop_ || !tasks_.empty(); });
            if (stop_ && tasks_.empty())
                return;
            task = std::move(tasks_.front());
            tasks_.pop();
        }

        try { task(); } catch(...) {}

        bool allDone;
        {
            std::lock_guard<std::mutex> locker(mtx_);
            allDone = (--unfinished_ == 0);
        }
        if (allDone)
            allDoneCv_.notify_all();
    }
}

void ThreadPool::wait()
{
    std::unique_lock<std::mutex> locker(mtx_);
    allDoneCv_.wait(locker, [this]() { return unfinished_ == 0; });
}

void ThreadPool::shutdown()
{
    {
        std::lock_guard<std::mutex> locker(mtx_);
        stop_ = true;
    }
    stopOrNotEmptyCv_.notify_all();
    for (auto& th : workers_)
        if (th.joinable()) th.join();
}
