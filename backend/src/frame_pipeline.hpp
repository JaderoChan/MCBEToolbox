#pragma once

#include <stdio.h> // fprintf
#include <atomic>  // std::atomic
#include <deque>   // std::deque
#include <future>  // std::future
#include <utility> // std::pair

#include <opencv2/core/mat.hpp> // cv::Mat

#include "image_ostream.hpp"
#include "thread_pool.hpp"

/**
 * 以有界滑动窗口并行处理帧数据流。每提交一个新任务后，若挂起（已提交但结果尚未被消费）的任务数量
 * 超过窗口大小，则消费最早的一个任务结果，从而将内存峰值占用限制在窗口大小以内。
 *
 * @tparam Product 单帧任务的结果类型。
 *
 * @param windowSizeFactor 滑动窗口大小相对于线程数的倍率（窗口大小 = numThreads * windowSizeFactor）。
 * @param logPrefix        日志信息前缀，用于标识调用来源。
 * @param processFrame     帧处理函数，签名：(const cv::Mat& frame, std::atomic<bool>& shouldStop) -> Product
 * @param reportProgress   进度回调函数，签名：(std::size_t completedCount) -> bool
 * @param consumeResult    结果处理函数，签名：(Product&&) -> bool
 * @return 全部帧均成功处理并消费返回 true，否则返回 false。
 */
template<typename Product, typename ProcessFrame, typename ReportProgress, typename ConsumeResult>
bool runFramePipeline(
    ImageOStream&    stream,
    int              numThreads,
    std::size_t      windowSizeFactor,
    const char*      logPrefix,
    ProcessFrame&&   processFrame,
    ReportProgress&& reportProgress,
    ConsumeResult&&  consumeResult)
{
    std::deque<std::future<Product>> pending;
    std::atomic<std::size_t> completedCount{0};
    std::atomic<bool>        shouldStop{false};

    ThreadPool threadPool(numThreads);
    const std::size_t windowSize = static_cast<std::size_t>(numThreads) * windowSizeFactor;

    // 消费窗口中最早的一个任务结果
    auto consumeOne = [&]() -> bool
    {
        if (shouldStop.load(std::memory_order_relaxed))
            return false;

        Product product = pending.front().get();
        pending.pop_front();

        return consumeResult(std::move(product));
    };

    while (!stream.isEnd())
    {
        if (shouldStop.load(std::memory_order_relaxed))
            return false;

        const cv::Mat frame = stream.nextFrame();
        if (frame.empty())
        {
            if (stream.isEnd())
                break;
            fprintf(stderr, "%s Empty frame be got, skip it\n", logPrefix);
            continue;
        }

        pending.emplace_back(threadPool.submit([&, frame, logPrefix]()
        {
            if (shouldStop.load(std::memory_order_relaxed))
                return Product();

            try
            {
                Product product = processFrame(frame, shouldStop);

                const std::size_t current = completedCount.fetch_add(1, std::memory_order_relaxed) + 1;
                if (reportProgress(current))
                    shouldStop.store(true, std::memory_order_relaxed);

                return product;
            }
            catch (std::exception& e)
            {
                fprintf(
                    stderr,
                    "%s Error occurred when other thread execute processFrame(), error message is '%s'\n",
                    logPrefix, e.what()
                );
                return Product();
            }
        }));

        if (pending.size() > windowSize && !consumeOne())
            return false;
    }

    while (!pending.empty())
    {
        if (!consumeOne())
            return false;
    }

    return true;
}
