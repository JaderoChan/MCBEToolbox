#include "mcstructure_factory.hpp"

#include <assert.h>      // assert
#include <limits.h>      // INT_MAX
#include <atomic>        // std::atomic
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map

#include <opencv2/core/mat.hpp>     // cv::Mat
#include <mcnbt/be/mcstructure.hpp> // nbt::be::MCStructure

#include "thread_pool.hpp"

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

std::vector<nbt::Tag> MCStructureFactory::generateDetachMCStructure(VideoFramesOStream& stream, int numThreads)
{
    reset();
    if (!stream.isOpened() || stream.isEnd() || !isConfigured() || numThreads < 1)
        return std::vector<nbt::Tag>();

    const auto numFrames = stream.frameCount();

    ThreadPool threadPool(numThreads);

    using TaskResult = std::pair<nbt::Tag, BlockUsageMap>;
    std::vector<std::future<TaskResult>> results;

    std::atomic<std::size_t> completed{0};
    std::atomic<bool>        shouldStop{false};

    while (!stream.isEnd())
    {
        if (shouldStop.load(std::memory_order_relaxed))
            break;

        cv::Mat frame = stream.nextFrame();
        if (frame.empty())
            continue;

        results.emplace_back(threadPool.submit([=, &completed, &shouldStop]()
        {
            if (shouldStop.load(std::memory_order_relaxed))
                return TaskResult();

            ImageFramesOStream imageStream(frame);
            BlockUsageMap localUsageCount;
            nbt::Tag mcstructure = generateSingleMCStructureHelper(
                imageStream, [](std::size_t, std::size_t, bool& stop, void* userdata) -> void
                { stop = static_cast<std::atomic<bool>*>(userdata)->load(std::memory_order_relaxed); },
                static_cast<void*>(&shouldStop), localUsageCount, false
            );

            const std::size_t current = completed.fetch_add(1, std::memory_order_relaxed) + 1;
            if (executeCallback(current, numFrames))
                shouldStop.store(true);

            return TaskResult(std::move(mcstructure), std::move(localUsageCount));
        }));
    }

    std::vector<nbt::Tag> ret;
    ret.reserve(results.size());
    for (auto& fut : results)
    {
        auto [mcstructure, localUsageCount] = fut.get();
        if (mcstructure.type() == nbt::TT_END)
            return std::vector<nbt::Tag>();
        for (auto& [id, count] : localUsageCount)
            updateBlockUsageCount(id, count);
        ret.push_back(std::move(mcstructure));
    }

    return ret;
}

nbt::Tag MCStructureFactory::generateSingleMCStructureHelper(
    FramesOStream&   stream,
    ProgressCallback callback,
    void*            userdata,
    BlockUsageMap&   blockUsageCount,
    bool             useFrameIndexCallback)
{
    if (!stream.isOpened() || stream.isEnd() || stream.frameCount() > INT_MAX || !isConfigured())
        return nbt::Tag();

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
            return nbt::Tag();
    }

    const int xs = mcstructure.size()[0].getInt();
    const int ys = mcstructure.size()[1].getInt();
    const int zs = mcstructure.size()[2].getInt();
    const std::size_t total =
        static_cast<std::size_t>(xs) *
        static_cast<std::size_t>(ys) *
        static_cast<std::size_t>(zs);
    mcstructure.blockIndices1().getList().resize(total, nbt::Tag(-1));
    mcstructure.blockIndices2().getList().resize(total, nbt::Tag(-1));

    // 缓存调色板方块及其名称与索引
    std::vector<const BlockData*> paletteCache;
    std::unordered_map<std::string_view, std::size_t> paletteIdxCache;

    auto& indices = mcstructure.blockIndices1();
    for (int num = 0; num < n; ++num)
    {
        cv::Mat frame = stream.nextFrame();
        if (frame.empty())
            return nbt::Tag();

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
                int x = 0, y = 0, z = 0;
                switch (desiredSurface_)
                {
                    case SURFACE_DIRECTION_UP:
                        x = w - col - 1;
                        y = n - num - 1;
                        z = h - row - 1;
                        break;
                    case SURFACE_DIRECTION_BOTTOM:
                        x = w - col - 1;
                        y = num;
                        z = row;
                        break;
                    case SURFACE_DIRECTION_NORTH:
                        x = w - col - 1;
                        y = h - row - 1;
                        z = num;
                        break;
                    case SURFACE_DIRECTION_SOUTH:
                        x = col;
                        y = h - row - 1;
                        z = n - num - 1;
                        break;
                    case SURFACE_DIRECTION_EAST:
                        x = num;
                        y = h - row - 1;
                        z = w - col - 1;
                        break;
                    case SURFACE_DIRECTION_WEST:
                        x = n - num - 1;
                        y = h - row - 1;
                        z = col;
                        break;
                    default:
                        return nbt::Tag();
                }

                // 计算索引值
                const std::size_t indicesIdx = x * zs * ys + y * zs + z;
                indices[indicesIdx] = nbt::Tag(paletteIdx);

                // 更新方块用量信息
                updateBlockUsageCount(blockUsageCount, id, 1);

                if (!useFrameIndexCallback)
                {
                    if (executeCallback(callback, userdata, num * h * w + row * w + col + 1, total))
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

    return mcstructure.root;
}
