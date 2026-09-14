#include "mcstructure_factory.hpp"

#include <assert.h>      // assert
#include <limits.h>      // INT_MAX
#include <stdio.h>       // fprintf
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map

#include <opencv2/core/mat.hpp>     // cv::Mat
#include <mcnbt/be/mcstructure.hpp> // nbt::be::MCStructure

#include "frame_pipeline.hpp"

MCStructureFactory::MCStructureFactory(
    const BlockDataMap& blocks,
    SurfaceDirection    desiredSurface,
    const Version&      blockFormatVersion)
    : BaseFactory(blocks, desiredSurface), blockFormatVersion_(blockFormatVersion)
{}

nbt::Tag MCStructureFactory::generateSingleMCStructure(ImageFramesOStream& stream)
{
    reset();
    return generateSingleMCStructureHelper(stream, callback_, userdata_, blockUsageCount_, false);
}

nbt::Tag MCStructureFactory::generateSingleMCStructure(VideoFramesOStream& stream)
{
    reset();
    return generateSingleMCStructureHelper(stream, callback_, userdata_, blockUsageCount_, true);
}

std::vector<nbt::Tag> MCStructureFactory::generateDetachMCStructure(FramesOStream& stream, int numThreads)
{
    reset();
    if (!stream.isOpened() || stream.isEnd() || !isConfigured() || numThreads < 1)
    {
        if (!stream.isOpened())
            fprintf(stderr, "MCStructureFactory::generateDetachMCStructure() Stream is not opened\n");
        if (!stream.isEnd())
            fprintf(stderr, "MCStructureFactory::generateDetachMCStructure() Stream is arrive end\n");
        if (!isConfigured())
            fprintf(stderr, "MCStructureFactory::generateDetachMCStructure() Factory is not configured\n");
        if (numThreads < 1)
            fprintf(stderr, "MCStructureFactory::generateDetachMCStructure() Parameter 'numThreads' is less than 1\n");
        return std::vector<nbt::Tag>();
    }
    assert(stream.frameCount() > 0);

    const auto numFrames = stream.frameCount();

    std::vector<nbt::Tag> ret;
    ret.reserve(static_cast<std::size_t>(numFrames));

    const bool ok = runFramePipeline<std::pair<nbt::Tag, BlockUsageMap>>(
        stream,
        numThreads,
        TASK_WINDOW_SIZE_FACTOR,
        "MCStructureFactory::generateDetachMCStructure()",
        [this](const cv::Mat& frame, std::atomic<bool>& shouldStop) { return processDetachFrame(frame, shouldStop); },
        [this, numFrames](std::size_t current) { return executeCallback(current, numFrames); },
        [this, &ret](std::pair<nbt::Tag, BlockUsageMap>&& result)
        {
            auto& [mcstructure, localUsageCount] = result;
            if (mcstructure.type() == nbt::TT_END)
            {
                fprintf(stderr, "MCStructureFactory::generateDetachMCStructure() Got a unexpected invalid NBT tag\n");
                return false;
            }

            for (auto& [id, count] : localUsageCount)
                updateBlockUsageCount(id, count);

            ret.push_back(std::move(mcstructure));
            return true;
        }
    );

    return ok ? std::move(ret) : std::vector<nbt::Tag>();
}

bool MCStructureFactory::generateDetachMCStructure(FramesOStream& stream, const std::string& outDirPath, int numThreads)
{
    reset();
    if (!stream.isOpened() || stream.isEnd() || !isConfigured() || numThreads < 1)
    {
        if (!stream.isOpened())
            fprintf(stderr, "MCStructureFactory::generateDetachMCStructure() Stream is not opened\n");
        if (!stream.isEnd())
            fprintf(stderr, "MCStructureFactory::generateDetachMCStructure() Stream is arrive end\n");
        if (!isConfigured())
            fprintf(stderr, "MCStructureFactory::generateDetachMCStructure() Factory is not configured\n");
        if (numThreads < 1)
            fprintf(stderr, "MCStructureFactory::generateDetachMCStructure() Parameter 'numThreads' is less than 1\n");
        return false;
    }
    if (!ensureDirectoryExists(outDirPath, "MCStructureFactory::generateDetachMCStructure()"))
        return false;
    assert(stream.frameCount() > 0);

    const auto numFrames = stream.frameCount();

    std::size_t frameIdx = 0;
    return runFramePipeline<std::pair<nbt::Tag, BlockUsageMap>>(
        stream,
        numThreads,
        TASK_WINDOW_SIZE_FACTOR,
        "MCStructureFactory::generateDetachMCStructure()",
        [this](const cv::Mat& frame, std::atomic<bool>& shouldStop) { return processDetachFrame(frame, shouldStop); },
        [this, numFrames](std::size_t current) { return executeCallback(current, numFrames); },
        [this, &outDirPath, &frameIdx](std::pair<nbt::Tag, BlockUsageMap>&& result)
        {
            auto& [mcstructure, localUsageCount] = result;
            if (mcstructure.type() == nbt::TT_END)
            {
                fprintf(
                    stderr,
                    "MCStructureFactory::generateDetachMCStructure() Got a unexpected invalid NBT tag\n"
                );
                return false;
            }

            for (auto& [id, count] : localUsageCount)
                updateBlockUsageCount(id, count);

            mcstructure.dump(outDirPath + "/" + std::to_string(frameIdx) + ".mcstructure", false);
            ++frameIdx;
            return true;
        }
    );
}

std::pair<nbt::Tag, BaseFactory::BlockUsageMap>
MCStructureFactory::processDetachFrame(const cv::Mat& frame, std::atomic<bool>& shouldStop)
{
    ImageFramesOStream imageStream(frame);
    BlockUsageMap localUsageCount;
    nbt::Tag mcstructure = generateSingleMCStructureHelper(
        imageStream, [](std::size_t, std::size_t, bool& stop, void* userdata) -> void
        { stop = static_cast<std::atomic<bool>*>(userdata)->load(std::memory_order_relaxed); },
        static_cast<void*>(&shouldStop), localUsageCount, true
    );
    return {std::move(mcstructure), std::move(localUsageCount)};
}

nbt::Tag MCStructureFactory::generateSingleMCStructureHelper(
    FramesOStream&   stream,
    ProgressCallback callback,
    void*            userdata,
    BlockUsageMap&   blockUsageCount,
    bool             useFrameIndexCallback)
{
    if (!stream.isOpened() || stream.isEnd() || stream.frameCount() > INT_MAX || !isConfigured())
    {
        if (!stream.isOpened())
            fprintf(stderr, "MCStructureFactory::generateSingleMCStructureHelper() Stream is not opened\n");
        if (stream.isEnd())
            fprintf(stderr, "MCStructureFactory::generateSingleMCStructureHelper() Stream is arrive end\n");
        if (stream.frameCount() > INT_MAX)
            fprintf(
                stderr,
                "MCStructureFactory::generateSingleMCStructureHelper() Stream's frame count (%lld) is too much\n",
                stream.frameCount()
            );
        if (!isConfigured())
            fprintf(stderr, "MCStructureFactory::generateSingleMCStructureHelper() Factory is not configured\n");
        return nbt::Tag();
    }

    nbt::be::MCStructure mcstructure(1, 1, 1, 1);
    const int n = static_cast<int>(stream.frameCount());
    const int w = stream.frameSize().width;
    const int h = stream.frameSize().height;
    switch (desiredSurface_)
    {
        case SURFACE_DIRECTION_UP:      // Fallthrough
        case SURFACE_DIRECTION_BOTTOM:
            mcstructure.size()[0] = w;
            mcstructure.size()[1] = n;
            mcstructure.size()[2] = h;
            break;
        case SURFACE_DIRECTION_NORTH:   // Fallthrough
        case SURFACE_DIRECTION_SOUTH:
            mcstructure.size()[0] = w;
            mcstructure.size()[1] = h;
            mcstructure.size()[2] = n;
            break;
        case SURFACE_DIRECTION_EAST:    // Fallthrough
        case SURFACE_DIRECTION_WEST:
            mcstructure.size()[0] = n;
            mcstructure.size()[1] = h;
            mcstructure.size()[2] = w;
            break;
        default:
            fprintf(
                stderr,
                "MCStructureFactory::generateSingleMCStructureHelper() The desired surface (%d) is invalid\n",
                desiredSurface_
            );
            return nbt::Tag();
    }

    const int xs = mcstructure.size()[0].getInt();
    const int ys = mcstructure.size()[1].getInt();
    const int zs = mcstructure.size()[2].getInt();
    std::size_t current = 0;
    const std::size_t total =
        static_cast<std::size_t>(xs) *
        static_cast<std::size_t>(ys) *
        static_cast<std::size_t>(zs);
    mcstructure.blockIndices1().getList().resize(total, nbt::Tag(-1));
    mcstructure.blockIndices2().getList().resize(total, nbt::Tag(-1));

    // 缓存调色板方块及其名称与索引
    std::vector<const BlockData*> paletteCache;
    std::unordered_map<std::string_view, int> paletteIdxCache;

    auto& indices = mcstructure.blockIndices1();
    for (int num = 0; num < n; ++num)
    {
        const cv::Mat frame = stream.nextFrame();
        if (frame.empty())
        {
            fprintf(
                stderr,
                "MCStructureFactory::generateSingleMCStructureHelper() The frame got from stream is empty\n"
            );
            return nbt::Tag();
        }

        assert(frame.cols == w && frame.rows == h);
        assert(frame.type() == CV_8UC4);

        for (int row = 0; row < h; ++row)
        {
            for (int col = 0; col < w; ++col)
            {
                const auto& color = frame.at<cv::Vec4b>(row, col);
                auto [id, block] = retrieveAppropriateBlock(color);

                // 默认为结构空位
                int paletteIdx = -1;
                if (block)
                {
                    if (paletteIdxCache.find(id) == paletteIdxCache.end())
                    {
                        paletteCache.push_back(block);
                        paletteIdx = static_cast<int>(paletteCache.size()) - 1;
                        paletteIdxCache[id] = paletteIdx;
                    }
                    else
                    {
                        paletteIdx = paletteIdxCache[id];
                    }
                }

                // 转换坐标系
                const auto pos = computePosition(col, row, num, w, h, n);
                const int x = pos[0];
                const int y = pos[1];
                const int z = pos[2];

                // 计算索引值
                const std::size_t indicesIdx =
                    static_cast<std::size_t>(x) * static_cast<std::size_t>(zs) * static_cast<std::size_t>(ys) +
                    static_cast<std::size_t>(y) * static_cast<std::size_t>(zs) +
                    static_cast<std::size_t>(z);
                indices[indicesIdx] = nbt::Tag(paletteIdx);

                // 更新方块用量信息
                if (block) updateBlockUsageCount(blockUsageCount, id, 1);

                if (!useFrameIndexCallback)
                {
                    ++current;
                    if (executeCallback(callback, userdata, current, total))
                        return nbt::Tag();
                }
            }
        }

        if (useFrameIndexCallback)
        {
            if (executeCallback(callback, userdata, num + 1, n))
                return nbt::Tag();
        }
    }

    // 填充调色板
    const int versionValue = static_cast<int>(blockFormatVersion_.toUInt32());
    auto& palette = mcstructure.blockPalette();
    for (const auto& block : paletteCache)
    {
        nbt::Tag paletteItem = nbt::Tag::compound();
        paletteItem["name"]    = block->id;
        paletteItem["states"]  = nbt::Tag::compound();
        paletteItem["version"] = versionValue;
        palette.pushBack(std::move(paletteItem));
    }

    return std::move(mcstructure.root);
}
