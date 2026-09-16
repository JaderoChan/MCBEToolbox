#include "mcfunction_factory.hpp"

#include <assert.h>    // assert
#include <limits.h>    // INT_MAX
#include <stdio.h>     // fprintf, snprintf
#include <array>       // std::array
#include <fstream>     // std::ofstream
#include <string_view> // std::string_view

#include "frame_pipeline.hpp"

MCFunctionFactory::MCFunctionFactory(const BlockDataMap& blocks, SurfaceDirection desiredSurface)
    : BaseFactory(blocks, desiredSurface)
{}

MCFunctionFactory::MCFunction MCFunctionFactory::generateSingleMCFunction(FramesOStream& stream, bool callbackPerFrame)
{
    reinitializeState();
    return generateSingleMCFunctionHelper(stream, callback_, userdata_, blockUsageMap_, callbackPerFrame);
}

std::vector<MCFunctionFactory::MCFunction>
MCFunctionFactory::generateDetachMCFunction(FramesOStream& stream, int numThreads)
{
    reinitializeState();
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

    std::vector<MCFunction> ret;
    ret.reserve(static_cast<std::size_t>(numFrames));

    const bool ok = runFramePipeline<std::pair<MCFunction, BlockUsageMap>>(
        stream,
        numThreads,
        TASK_WINDOW_SIZE_FACTOR,
        "MCFunctionFactory::generateDetachMCFunction()",
        [this](const cv::Mat& frame, std::atomic<bool>& shouldStop) { return processDetachFrame(frame, shouldStop); },
        [this, numFrames](std::size_t current) { return invokeProgressCallback(current, numFrames); },
        [this, &ret](std::pair<MCFunction, BlockUsageMap>&& result)
        {
            auto& [mcfunction, localUsageCount] = result;

            for (auto& [id, count] : localUsageCount)
                updateBlockUsageMap(id, count);

            ret.push_back(std::move(mcfunction));
            return true;
        }
    );

    return ok ? std::move(ret) : std::vector<MCFunction>();
}

bool MCFunctionFactory::generateDetachMCFunction(FramesOStream& stream, const std::string& outDirPath, int numThreads)
{
    reinitializeState();
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
        return false;
    }
    if (!ensureDirectoryExists(outDirPath, "MCFunctionFactory::generateDetachMCFunction()"))
        return false;
    assert(stream.frameCount() > 0);

    const auto numFrames = stream.frameCount();

    std::size_t frameIdx = 1;
    return runFramePipeline<std::pair<MCFunction, BlockUsageMap>>(
        stream,
        numThreads,
        TASK_WINDOW_SIZE_FACTOR,
        "MCFunctionFactory::generateDetachMCFunction()",
        [this](const cv::Mat& frame, std::atomic<bool>& shouldStop) { return processDetachFrame(frame, shouldStop); },
        [this, numFrames](std::size_t current) { return invokeProgressCallback(current, numFrames); },
        [this, &outDirPath, &frameIdx](std::pair<MCFunction, BlockUsageMap>&& result)
        {
            auto& [mcfunction, localUsageCount] = result;

            for (auto& [id, count] : localUsageCount)
                updateBlockUsageMap(id, count);

            std::size_t subIdx = 0;
            char buf[256];
            std::ofstream file;
            for (std::size_t i = 0; i < mcfunction.size(); ++i)
            {
                if (i % MAX_COMMAND_COUNT_PER_MCFUNCTION == 0)
                {
                    ++subIdx;
                    snprintf(buf, sizeof(buf), "%s/%zu-%zu.mcfunction", outDirPath.c_str(), frameIdx, subIdx);
                    if (file.is_open()) file.close();
                    file = std::ofstream(buf);
                    if (!file.is_open())
                    {
                        fprintf(
                            stderr,
                            "MCFunctionFactory::generateDetachMCFunction() Failed to open the out file '%s'\n",
                            buf
                        );
                        return false;
                    }
                }

                file << mcfunction[i] << '\n';
            }

            ++frameIdx;
            return true;
        }
    );
}

std::pair<MCFunctionFactory::MCFunction, BaseFactory::BlockUsageMap>
MCFunctionFactory::processDetachFrame(const cv::Mat& frame, std::atomic<bool>& shouldStop)
{
    ImageFramesOStream imageStream(frame);
    BlockUsageMap localUsageCount;
    MCFunction mcfunction = generateSingleMCFunctionHelper(
        imageStream, [](std::size_t, std::size_t, bool& stop, void* userdata) -> void
        { stop = static_cast<std::atomic<bool>*>(userdata)->load(std::memory_order_relaxed); },
        static_cast<void*>(&shouldStop), localUsageCount, true
    );
    return {std::move(mcfunction), std::move(localUsageCount)};
}

MCFunctionFactory::MCFunction MCFunctionFactory::generateSingleMCFunctionHelper(
    FramesOStream&   stream,
    ProgressCallback callback,
    void*            userdata,
    BlockUsageMap&   blockUsageMap,
    bool             callbackPerFrame)
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
                        updateBlockUsageMap(blockUsageMap, lastId, col - lastCol);
                    }
                    lastId    = id;
                    lastBlock = block;
                    lastCol   = col;
                }

                if (!callbackPerFrame)
                {
                    ++current;
                    if (invokeProgressCallback(callback, current, total, userdata))
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
                updateBlockUsageMap(blockUsageMap, lastId, w - lastCol);
            }
        }

        if (callbackPerFrame)
        {
            if (invokeProgressCallback(callback, num + 1, n, userdata))
                return MCFunction();
        }
    }

    return mcfunction;
}
