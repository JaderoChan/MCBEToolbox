/**
 * @file thread_pool.hpp
 * @author 𬱖珞JaderoChan
 * @date 2026-08-19
 *
 * 一个简易的线程池实现，支持 C++11 及以上。使用方法可以参见 example 程序。
 */

#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <cstddef>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

#ifdef _MSC_VER
    #define _THREADPOOL_CPPVERSION _MSVC_LANG
#else
    #define _THREADPOOL_CPPVERSION __cplusplus
#endif

/**
 * @todo
 * - 引入 “核心线程” 与 “非核心线程” 实现，同步增加相应 feature。
 * - 加入状态查询
 * - 限制队列大小，并处理任务拒绝情况
 */
class ThreadPool
{
public:
    explicit ThreadPool(std::size_t numThreads);
    ~ThreadPool();

    /** @brief 提交一个任务至队列中。 */
    template<typename F, typename... Args>
    // result_of 在 C++17 中被标记为弃用并在 C++20 中彻底移除。在 C++17 及以上版本中使用 invoke_result_t 替代 result_of。
#if _THREADPOOL_CPPVERSION >= 201703L
    std::future<std::invoke_result_t<F, Args...>> submit(F&& f, Args&&... args);
#elif _THREADPOOL_CPPVERSION >= 201103L
    std::future<typename std::result_of<F(Args...)>::type> submit(F&& f, Args&&... args);
#else
    #error "At least C++11 is necessary."
#endif

    /** @brief 等待所有任务（包括队列中的任务）执行完成。 */
    void wait();
    /** @brief 退出所有线程。 */
    void shutdown();

private:
    void work();

    std::vector<std::thread>           workers_;            // 工作线程列表
    std::queue<std::function<void ()>> tasks_;              // 工作任务队列

    mutable std::mutex                 mtx_;
    mutable std::condition_variable    stopOrNotEmptyCv_;   // 即将停止或任务队列不为空时发出通知
    mutable std::condition_variable    allDoneCv_;          // 所有任务执行完之后发出通知

    bool        stop_;
    std::size_t unfinished_;

};

template<typename F, typename ...Args>
#if _THREADPOOL_CPPVERSION >= 201703L
inline std::future<std::invoke_result_t<F, Args...>> ThreadPool::submit(F&& f, Args&&... args)
{
    using ReturnType = std::invoke_result_t<F, Args...>;

    auto taskPtr = std::make_shared<std::packaged_task<ReturnType ()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    std::future<ReturnType> fut = taskPtr->get_future();

    std::lock_guard<std::mutex> locker(mtx_);
    if (stop_)
        throw std::runtime_error("Cannot submit task after thread pool was stopped.");
    tasks_.emplace([taskPtr]() { std::invoke(*taskPtr); });
    ++unfinished_;

    stopOrNotEmptyCv_.notify_one();
    return fut;
}
#elif _THREADPOOL_CPPVERSION >= 201103L
inline std::future<typename std::result_of<F(Args...)>::type> ThreadPool::submit(F&& f, Args && ...args)
{
    using ReturnType = typename std::result_of<F(Args...)>::type;

    auto taskPtr = std::make_shared<std::packaged_task<ReturnType ()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    std::future<ReturnType> fut = taskPtr->get_future();

    std::lock_guard<std::mutex> locker(mtx_);
    if (stop_)
        throw std::runtime_error("Cannot submit task after thread pool was stopped.");
    tasks_.emplace([taskPtr]() { (*taskPtr)(); });
    ++unfinished_;

    stopOrNotEmptyCv_.notify_one();
    return fut;
}
#else
    #error "At least C++11 is necessary."
#endif

#undef _THREADPOOL_CPPVERSION

#endif // !THREAD_POOL_HPP
