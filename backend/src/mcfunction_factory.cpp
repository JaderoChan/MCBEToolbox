#include "mcfunction_factory.hpp"

#include <assert.h>    // assert
#include <limits.h>    // INT_MAX
#include <stdio.h>     // fprintf
#include <string.h>    // snprintf
#include <array>       // std::array
#include <atomic>      // std::atomic
#include <string_view> // std::string_view

#include "thread_pool.hpp"

MCFunctionFactory::MCFunctionFactory(const BlockDataMap& blocks, SurfaceDirection desiredSurface)
    : BaseFactory(blocks, desiredSurface)
{}

MCFunctionFactory::MCFunction MCFunctionFactory::generateSingleMCFunction(ImageFramesOStream& stream)
{
    reset();
    return generateSingleMCFunctionHelper(stream, callback_, userdata_, blockUsageCount_, false);
}

MCFunctionFactory::MCFunction MCFunctionFactory::generateSingleMCFunction(VideoFramesOStream& stream)
{
    reset();
    return generateSingleMCFunctionHelper(stream, callback_, userdata_, blockUsageCount_, true);
}

std::vector<MCFunctionFactory::MCFunction>
MCFunctionFactory::generateDetachMCFunction(FramesOStream& stream, int numThreads)
{
    reset();
    if (!stream.isOpened() || stream.isEnd() || !isConfigured() || numThreads < 1)
    {
        if (!stream.isOpened())
            fprintf(stderr, "MCFunctionFactory::generateDetachMCFunction() Stream is not opened\n");
        if (!stream.isEnd())
            fprintf(stderr, "MCFunctionFactory::generateDetachMCFunction() Stream is arrive end\n");
        if (!isConfigured())
            fprintf(stderr, "MCFunctionFactory::generateDetachMCFunction() Factory is not configured\n");
        if (numThreads < 1)
            fprintf(stderr, "MCFunctionFactory::generateDetachMCFunction() Parameter 'numThreads' is less than 1\n");
        return std::vector<MCFunction>();
    }
    assert(stream.frameCount() > 0);

    const auto numFrames = stream.frameCount();

    using TaskResult = std::pair<MCFunction, BlockUsageMap>;
    std::vector<std::future<TaskResult>> results;

    std::atomic<std::size_t> completed{0};
    std::atomic<bool>        shouldStop{false};

    ThreadPool threadPool(numThreads);

    while (!stream.isEnd())
    {
        if (shouldStop.load(std::memory_order_relaxed))
            break;

        const cv::Mat frame = stream.nextFrame();
        if (frame.empty())
        {
            if (stream.isEnd())
                break;
            fprintf(stderr, "MCFunctionFactory::generateDetachMCFunction() Empty frame be got, skip it\n");
            continue;
        }

        results.emplace_back(threadPool.submit([=, &completed, &shouldStop]()
        {
            if (shouldStop.load(std::memory_order_relaxed))
                return TaskResult();

            try
            {
                ImageFramesOStream imageStream(frame);
                BlockUsageMap localUsageCount;
                MCFunction MCFunction = generateSingleMCFunctionHelper(
                    imageStream, [](std::size_t, std::size_t, bool& stop, void* userdata) -> void
                    { stop = static_cast<std::atomic<bool>*>(userdata)->load(std::memory_order_relaxed); },
                    static_cast<void*>(&shouldStop), localUsageCount, true
                );

                const std::size_t current = completed.fetch_add(1, std::memory_order_relaxed) + 1;
                if (executeCallback(current, numFrames))
                    shouldStop.store(true);

                return TaskResult(std::move(MCFunction), std::move(localUsageCount));
            }
            catch (std::exception& e)
            {
                fprintf(
                    stderr,
                    "MCFunctionFactory::generateDetachMCFunction() "
                    "Error occurred when other thread execute generateSingleMCFunctionHelper(), "
                    "error message is '%s'\n",
                    e.what()
                );
                return TaskResult(MCFunction(), BlockUsageMap());
            }
        }));
    }

    if (shouldStop.load(std::memory_order_relaxed))
        return std::vector<MCFunction>();

    std::vector<MCFunction> ret;
    ret.reserve(results.size());
    for (auto& fut : results)
    {
        auto [mcfunction, localUsageCount] = fut.get();
        for (auto& [id, count] : localUsageCount)
            updateBlockUsageCount(id, count);
        ret.push_back(std::move(mcfunction));
    }

    return ret;
}

MCFunctionFactory::MCFunction MCFunctionFactory::generateSingleMCFunctionHelper(
    FramesOStream&   stream,
    ProgressCallback callback,
    void*            userdata,
    BlockUsageMap&   blockUsageCount,
    bool             useFrameIndexCallback)
{
    if (!stream.isOpened() || stream.isEnd() || stream.frameCount() > INT_MAX || !isConfigured())
    {
        if (!stream.isOpened())
            fprintf(stderr, "MCFunctionFactory::generateSingleMCFunctionHelper() Stream is not opened\n");
        if (stream.isEnd())
            fprintf(stderr, "MCFunctionFactory::generateSingleMCFunctionHelper() Stream is arrive end\n");
        if (stream.frameCount() > INT_MAX)
            fprintf(
                stderr,
                "MCFunctionFactory::generateSingleMCFunctionHelper() Stream's frame count (%lld) is too much\n",
                stream.frameCount()
            );
        if (!isConfigured())
            fprintf(stderr, "MCFunctionFactory::generateSingleMCFunctionHelper() Factory is not configured\n");
        return MCFunction();
    }

    const int n = static_cast<int>(stream.frameCount());
    const int w = stream.frameSize().width;
    const int h = stream.frameSize().height;
    std::size_t current = 0;
    const std::size_t total =
        static_cast<std::size_t>(n) *
        static_cast<std::size_t>(w) *
        static_cast<std::size_t>(h);
    MCFunction mcfunction;
    mcfunction.reserve(total);

    char buf[256];
    for (int num = 0; num < n; ++num)
    {
        cv::Mat frame = stream.nextFrame();
        if (frame.empty())
        {
            fprintf(
                stderr,
                "MCFunctionFactory::generateSingleMCFunctionHelper() The frame got from stream is empty\n"
            );
            return MCFunction();
        }

        assert(frame.cols == w && frame.rows == h);
        assert(frame.type() == CV_8UC4);

        for (int row = 0; row < h; ++row)
        {
            std::string_view lastId;
            const BlockData* lastBlock = nullptr;
            int lastCol = 0;
            for (int col = 0; col < w; ++col)
            {
                const auto& color = frame.at<cv::Vec4b>(row, col);
                auto [id, block] = retrieveAppropriateBlock(color);
                if (lastBlock == nullptr)
                {
                    lastId    = id;
                    lastBlock = block;
                    lastCol   = col;
                }

                if (lastBlock != block)
                {
                    if (lastBlock)
                    {
                        const auto pos1 = computePosition(lastCol, row, num, w, h, n);
                        const auto pos2 = computePosition(col - 1, row, num, w, h, n);
                        snprintf(
                            buf, sizeof(buf),
                            "fill ~%d ~%d ~%d ~%d ~%d ~%d %s",
                            pos1[0], pos1[1], pos1[2],
                            pos2[0], pos2[1], pos2[2],
                            lastBlock->id.c_str()
                        );
                        mcfunction.emplace_back(buf);

                        // 更新方块用量信息
                        updateBlockUsageCount(blockUsageCount, lastId, col - lastCol);
                    }
                    lastId    = id;
                    lastBlock = block;
                    lastCol   = col;
                }

                if (!useFrameIndexCallback)
                {
                    ++current;
                    if (executeCallback(callback, userdata, current, total))
                        return MCFunction();
                }
            }

            if (lastBlock)
            {
                const auto pos1 = computePosition(lastCol, row, num, w, h, n);
                const auto pos2 = computePosition(w - 1,   row, num, w, h, n);
                snprintf(
                    buf, sizeof(buf),
                    "fill ~%d ~%d ~%d ~%d ~%d ~%d %s",
                    pos1[0], pos1[1], pos1[2],
                    pos2[0], pos2[1], pos2[2],
                    lastBlock->id.c_str()
                );
                mcfunction.emplace_back(buf);

                // 更新方块用量信息
                updateBlockUsageCount(blockUsageCount, lastId, w - lastCol);
            }
        }

        if (useFrameIndexCallback)
        {
            if (executeCallback(callback, userdata, num + 1, n))
                return MCFunction();
        }
    }

    return mcfunction;
}
