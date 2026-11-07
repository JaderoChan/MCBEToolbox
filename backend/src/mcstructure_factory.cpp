#include "mcstructure_factory.hpp"

#include <assert.h>      // assert
#include <limits.h>      // INT_MAX
#include <stdio.h>       // fprintf
#include <exception>     // std::exception
#include <fstream>       // std::ofstream
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map

#include <opencv2/core/mat.hpp>     // cv::Mat
#include <mcnbt/be/mcstructure.hpp> // nbt::be::MCStructure

#include "format_string.hpp"
#include "frame_pipeline.hpp"

MCStructureFactory::MCStructureFactory(
    const BlockDataMap& blocks,
    SurfaceDirection    desiredSurface,
    const Version&      blockFormatVersion)
    : BaseFactory(blocks, desiredSurface), blockFormatVersion_(blockFormatVersion)
{}

BaseFactory::ResultType MCStructureFactory::generateSingleMCStructure(
    ImageOStream& stream, bool callbackPerFrame, nbt::Tag& outStructure)
{
    reinitializeState();
    return generateSingleMCStructureHelper(
        stream, callback_, userdata_, blockUsageMap_, callbackPerFrame, outStructure);
}

BaseFactory::ResultType MCStructureFactory::generateDetachMCStructure(
    ImageOStream& stream, int numThreads, std::vector<nbt::Tag>& outStructures)
{
    reinitializeState();
    if (!validatePreconditions(stream, "MCStructureFactory::generateDetachMCStructure()", numThreads))
        return FAIL;
    assert(stream.frameCount() > 0);

    const auto numFrames = stream.frameCount();

    std::vector<nbt::Tag> ret;
    ret.reserve(static_cast<std::size_t>(numFrames));

    bool cancelled = false;
    const bool ok = runFramePipeline<std::pair<nbt::Tag, BlockUsageMap>>(
        stream,
        numThreads,
        TASK_WINDOW_SIZE_FACTOR,
        "MCStructureFactory::generateDetachMCStructure()",
        [this](const cv::Mat& frame, std::atomic<bool>& shouldStop) { return processDetachFrame(frame, shouldStop); },
        [this, numFrames](std::size_t current) { return invokeProgressCallback(current, numFrames); },
        [this, &ret](std::pair<nbt::Tag, BlockUsageMap>&& result)
        {
            auto& [mcstructure, localUsageCount] = result;
            if (mcstructure.type() == nbt::TT_END)
            {
                fprintf(stderr, "MCStructureFactory::generateDetachMCStructure() Got a unexpected invalid NBT tag\n");
                return false;
            }

            for (auto& [id, count] : localUsageCount)
                updateBlockUsageMap(id, count);

            ret.push_back(std::move(mcstructure));
            return true;
        },
        &cancelled
    );

    if (!ok)
    {
        if (cancelled)
            return CANCEL;
        if (errorMessage_.empty())
        {
            errorMessage_ = "MCStructureFactory::generateDetachMCStructure() Failed to process one or more frames";
            fprintf(stderr, "%s\n", errorMessage_.c_str());
        }
        return FAIL;
    }

    outStructures = std::move(ret);
    return SUCCESS;
}

BaseFactory::ResultType MCStructureFactory::generateDetachMCStructure(
    ImageOStream& stream, const std::filesystem::path& outDirPath,
    const std::filesystem::path& fileName, int numThreads)
{
    reinitializeState();
    if (!validatePreconditions(stream, "MCStructureFactory::generateDetachMCStructure()", numThreads))
        return FAIL;
    if (!ensureDirectoryExists(outDirPath, "MCStructureFactory::generateDetachMCStructure()"))
        return FAIL;
    assert(stream.frameCount() > 0);

    const auto numFrames = stream.frameCount();

    std::size_t frameIdx = 1;
    bool cancelled = false;
    const bool ok = runFramePipeline<std::pair<nbt::Tag, BlockUsageMap>>(
        stream,
        numThreads,
        TASK_WINDOW_SIZE_FACTOR,
        "MCStructureFactory::generateDetachMCStructure()",
        [this](const cv::Mat& frame, std::atomic<bool>& shouldStop) { return processDetachFrame(frame, shouldStop); },
        [this, numFrames](std::size_t current) { return invokeProgressCallback(current, numFrames); },
        [this, &outDirPath, &fileName, &frameIdx](std::pair<nbt::Tag, BlockUsageMap>&& result)
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
                updateBlockUsageMap(id, count);

            const std::filesystem::path path =
                outDirPath / fs_utils::pathFromUtf8(fileName.u8string() + "-" + std::to_string(frameIdx) + ".mcstructure");
            std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
            if (!ofs)
            {
                errorMessage_ = formatString(
                    "MCStructureFactory::generateDetachMCStructure() Failed to create the file '{}'", path.string());
                fprintf(stderr, "%s\n", errorMessage_.c_str());
                return false;
            }
            try
            {
                mcstructure.dump(ofs, false);
            }
            catch (const std::exception& e)
            {
                errorMessage_ = formatString(
                    "MCStructureFactory::generateDetachMCStructure() Failed to dump the mcstructure: {}", e.what());
                fprintf(stderr, "%s\n", errorMessage_.c_str());
                return false;
            }
            ++frameIdx;
            return true;
        },
        &cancelled
    );

    if (!ok)
    {
        if (cancelled)
            return CANCEL;
        if (errorMessage_.empty())
        {
            errorMessage_ = "MCStructureFactory::generateDetachMCStructure() Failed to process one or more frames";
            fprintf(stderr, "%s\n", errorMessage_.c_str());
        }
        return FAIL;
    }
    return SUCCESS;
}

std::pair<nbt::Tag, BaseFactory::BlockUsageMap>
MCStructureFactory::processDetachFrame(const cv::Mat& frame, std::atomic<bool>& shouldStop)
{
    SingleImageOStream imageStream(frame);
    BlockUsageMap      localUsageCount;
    nbt::Tag           mcstructure;
    generateSingleMCStructureHelper(
        imageStream, [](std::size_t, std::size_t, bool& stop, void* userdata) -> void
        { stop = static_cast<std::atomic<bool>*>(userdata)->load(std::memory_order_relaxed); },
        static_cast<void*>(&shouldStop), localUsageCount, true, mcstructure
    );
    return {std::move(mcstructure), std::move(localUsageCount)};
}

BaseFactory::ResultType MCStructureFactory::generateSingleMCStructureHelper(
    ImageOStream&    stream,
    ProgressCallback callback,
    void*            userdata,
    BlockUsageMap&   blockUsageMap,
    bool             callbackPerFrame,
    nbt::Tag&        outStructure)
{
    if (!validatePreconditions(stream, "MCStructureFactory::generateSingleMCStructureHelper()"))
        return FAIL;
    if (stream.frameCount() > INT_MAX)
    {
        errorMessage_ = formatString(
            "MCStructureFactory::generateSingleMCStructureHelper() Stream's frame count ({}) is too much",
            stream.frameCount()
        );
        fprintf(stderr, "%s\n", errorMessage_.c_str());
        return FAIL;
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
            errorMessage_ = formatString(
                "MCStructureFactory::generateSingleMCStructureHelper() The desired surface ({}) is invalid",
                static_cast<int>(desiredSurface_)
            );
            fprintf(stderr, "%s\n", errorMessage_.c_str());
            return FAIL;
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
            errorMessage_ = "MCStructureFactory::generateSingleMCStructureHelper() The frame got from stream is empty";
            fprintf(stderr, "%s\n", errorMessage_.c_str());
            return FAIL;
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
                if (block) updateBlockUsageMap(blockUsageMap, id, 1);

                if (!callbackPerFrame)
                {
                    ++current;
                    if (invokeProgressCallback(callback, current, total, userdata))
                        return CANCEL;
                }
            }
        }

        if (callbackPerFrame)
        {
            if (invokeProgressCallback(callback, num + 1, n, userdata))
                return CANCEL;
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

    outStructure = std::move(mcstructure.root);
    return SUCCESS;
}
