#include <block_image_factory.hpp>

#include <assert.h>   // assert
#include <stdio.h>    // fprintf
#include <atomic>     // std::atomic
#include <filesystem> // std::filesystem

#include <opencv2/imgcodecs.hpp> // cv::imread
#include <opencv2/imgproc.hpp>   // cv::cvtColor
#include <opencv2/videoio.hpp>   // cv::VideoWriter

#include <image_utilities.hpp>
#include "frame_pipeline.hpp"

BlockImageFactory::BlockImageFactory(
    const BlockDataMap& blocks,
    SurfaceDirection    desiredSurface,
    std::string_view    texturesDirPath)
    : BaseFactory(blocks, desiredSurface), texturesDirPath_(texturesDirPath)
{}

cv::Mat BlockImageFactory::generateBlockImage(ImageFramesOStream& stream)
{
    reset();
    return generateBlockImageHelper(stream, callback_, userdata_, blockUsageCount_, texturesCache_, false);
}

bool BlockImageFactory::generateBlockVideo(VideoFramesOStream& stream, const std::string& outFilePath, int numThreads)
{
    reset();

    if (!stream.isOpened() || stream.isEnd() || !isConfigured() || numThreads < 1)
    {
        if (!stream.isOpened())
            fprintf(stderr, "BlockImageFactory::generateBlockVideo() Stream is not opened\n");
        if (!stream.isEnd())
            fprintf(stderr, "BlockImageFactory::generateBlockVideo() Stream is arrive end\n");
        if (!isConfigured())
            fprintf(stderr, "BlockImageFactory::generateBlockVideo() Factory is not configured\n");
        if (numThreads < 1)
            fprintf(stderr, "BlockImageFactory::generateBlockVideo() Parameter 'numThreads' is less than 1\n");
        return false;
    }

    // 确保输出路径的父目录存在
    const std::filesystem::path outPath(outFilePath);
    if (outPath.has_parent_path())
    {
        if (!ensureDirectoryExists(outPath.parent_path().string(), "BlockImageFactory::generateBlockVideo()"))
            return false;
    }

    int fps = stream.fps();
    fps = fps > 0 ? fps : 25;

    cv::Size frameSize = stream.frameSize();
    if (frameSize.empty())
    {
        fprintf(
            stderr,
            "BlockImageFactory::generateBlockVideo() "
            "The frame size (%d * %d) got from stream is invalid\n",
            frameSize.width, frameSize.height
        );
        return false;
    }
    if (frameSize.width > VIDEO_FRAME_MAX_WIDTH || frameSize.height > VIDEO_FRAME_MAX_HEIGHT)
    {
        fprintf(
            stderr,
            "BlockImageFactory::generateBlockVideo() "
            "The frame size (%d * %d) is too large for block video, "
            "the maximum acceptable size is (%d * %d)\n",
            frameSize.width, frameSize.height,
            VIDEO_FRAME_MAX_WIDTH, VIDEO_FRAME_MAX_HEIGHT
        );
        return false;
    }

    frameSize *= 16;

    // 优先使用 H264 编码，若打不开（例如系统未注册对应的硬件/软件编码器），
    // 尝试改用 avc1 标签，输出文件名应该以 .mp4 为后缀
    const int fourccCandidates[] = {
        cv::VideoWriter::fourcc('H', '2', '6', '4'),
        cv::VideoWriter::fourcc('a', 'v', 'c', '1'),
    };

    cv::VideoWriter writer;
    for (const int fourcc : fourccCandidates)
    {
        writer.open(outFilePath, DEFAULT_VIDEO_BACKEND, fourcc, fps, frameSize);
        if (writer.isOpened())
            break;
    }
    if (!writer.isOpened())
    {
        fprintf(
            stderr,
            "BlockImageFactory::generateBlockVideo() "
            "Failed to open the video writer created by file '%s' with all attempted codecs, "
            "please check whether a H264 video encoder is available on this system\n",
            outFilePath.c_str()
        );
        return false;
    }

    const auto numFrames = stream.frameCount();

    using TaskResult = std::pair<cv::Mat, BlockUsageMap>;

    return runFramePipeline<TaskResult>(
        stream,
        numThreads,
        TASK_WINDOW_SIZE_FACTOR,
        "BlockImageFactory::generateBlockVideo()",
        [this](const cv::Mat& frame, std::atomic<bool>& shouldStop)
        {
            ImageFramesOStream imageStream(frame);
            BlockUsageMap   localUsageCount;
            BlockTextureMap localTexturesCache;
            cv::Mat blockImage = generateBlockImageHelper(
                imageStream, [](std::size_t, std::size_t, bool& stop, void* userdata) -> void
                { stop = static_cast<std::atomic<bool>*>(userdata)->load(std::memory_order_relaxed); },
                static_cast<void*>(&shouldStop), localUsageCount, localTexturesCache, true
            );
            return TaskResult(std::move(blockImage), std::move(localUsageCount));
        },
        [this, numFrames](std::size_t current)
        {
            return executeCallback(current, numFrames);
        },
        [this, &writer](TaskResult&& result)
        {
            auto& [blockImage, localUsageCount] = result;
            if (blockImage.empty())
            {
                fprintf(stderr, "BlockImageFactory::generateBlockVideo() Got a unexpected empty block image\n");
                return false;
            }

            for (auto& [id, count] : localUsageCount)
                updateBlockUsageCount(id, count);

            cv::Mat image;
            cv::cvtColor(blockImage, image, cv::COLOR_BGRA2BGR);
            writer.write(image);
            return true;
        }
    );
}

cv::Mat BlockImageFactory::generateBlockImageHelper(
    FramesOStream&   stream,
    ProgressCallback callback,
    void*            userdata,
    BlockUsageMap&   blockUsageCount,
    BlockTextureMap& texturesCache,
    bool             callbackPerFrame)
{
    if (!stream.isOpened() || stream.isEnd() || !isConfigured())
    {
        if (!stream.isOpened())
            fprintf(stderr, "BlockImageFactory::generateBlockImageHelper() Stream is not opened\n");
        if (stream.isEnd())
            fprintf(stderr, "BlockImageFactory::generateBlockImageHelper() Stream is arrive end\n");
        if (!isConfigured())
            fprintf(stderr, "BlockImageFactory::generateBlockImageHelper() Factory is not configured\n");
        return cv::Mat();
    }
    cv::Mat image = stream.nextFrame();
    if (image.empty())
    {
        fprintf(stderr, "BlockImageFactory::generateBlockImageHelper() The frame got from stream is empty\n");
        return cv::Mat();
    }

    assert(image.type() == CV_8UC4);

    std::size_t current = 0;
    const std::size_t total = static_cast<std::size_t>(image.rows) * static_cast<std::size_t>(image.cols);

    // 假定所有材质图片尺寸为 16*16，所以每个像素对应 16*16 的方块材质区域
    cv::Mat ret(image.rows * 16, image.cols * 16, CV_8UC4, cv::Scalar(0.0, 0.0, 0.0, 0.0));
    for (int row = 0; row < image.rows; ++row)
    {
        for (int col = 0; col < image.cols; ++col)
        {
            const auto color = image.at<cv::Vec4b>(row, col);
            auto [id, block] = retrieveAppropriateBlock(color);

            if (block)
            {
                const auto& surface = block->surfaceData.get(desiredSurface_);
                if (!surface.first.empty())
                {
                    const std::string texturePath = createTexturePath(surface.first);

                    // 如果当前材质还未被加载则将其加载至缓存中
                    if (texturesCache.find(texturePath) == texturesCache.end())
                    {
                        cv::Mat texture = cv::imread(texturePath, cv::IMREAD_UNCHANGED);
                        if (!texture.empty())
                            texture = convertColorToBgra(texture);
                        if (texture.empty())
                            texture = cv::Mat(16, 16, CV_8UC4, cv::Scalar(0.0, 0.0, 0.0, 0.0));
                        texturesCache[texturePath] = texture;
                    }

                    // 直接从缓存中加载方块材质
                    const cv::Mat texture = texturesCache[texturePath];
                    // 复制方块材质至像素映射区域
                    texture.copyTo(ret(
                        cv::Range(row * 16, row * 16 + 16),
                        cv::Range(col * 16, col * 16 + 16)
                    ));
                }

                // 更新方块用量信息
                updateBlockUsageCount(blockUsageCount, id, 1);
            }

            if (!callbackPerFrame)
            {
                ++current;
                if (executeCallback(callback, userdata, current, total))
                    return cv::Mat();
            }
        }
    }

    if (callbackPerFrame)
    {
        if (executeCallback(callback, userdata, 1, 1))
            return cv::Mat();
    }

    return ret;
}
