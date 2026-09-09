#include "frames_ostream.hpp"

#include "image_utilities.hpp"

cv::Mat FramesOStream::nextFrame()
{
    cv::Mat frame = readNextFrame();
    if (frame.empty())
        return frame;

    // 根据尺寸上限对原始帧做等比例缩放，尺寸与限制后的 frameSize() 一致时无需重复处理
    const cv::Size targetSize = frameSize();
    if (!targetSize.empty() && (frame.cols != targetSize.width || frame.rows != targetSize.height))
        frame = resizeImage(frame, targetSize);

    return frame;
}

cv::Size FramesOStream::frameSize() const
{
    if (cachedSize_.empty())
    {
        const cv::Size rawSize = readFrameSize();
        if (!rawSize.empty())
            cachedSize_ = limitSize(rawSize, maxw_, maxh_);
    }
    return cachedSize_;
}

cv::Mat SingleFramesOStream::readNextFrame()
{
    if (consumed_ || image_.empty())
        return cv::Mat();

    consumed_ = true;
    return image_;
}

cv::Size SingleFramesOStream::readFrameSize() const
{
    return image_.size();
}

long long VideoFramesOStream::frameCount() const
{
    if (!capture_.isOpened())
        return INVALID_INDEX;

    const double count = capture_.get(cv::CAP_PROP_FRAME_COUNT);
    const long long rawCount = count > 0 ? static_cast<long long>(count) : INVALID_INDEX;

    if (maxFrameCount_ < 0)
        return rawCount;
    if (rawCount == INVALID_INDEX)
        return maxFrameCount_;
    return rawCount < maxFrameCount_ ? rawCount : maxFrameCount_;
}

long long VideoFramesOStream::currentFrameIndex() const
{
    return currentFrameIndex_;
}

cv::Mat VideoFramesOStream::readNextFrame()
{
    if (!capture_.isOpened() || isEnd_)
        return cv::Mat();

    // 已达到 maxFrameCount 上限，视作流结束
    if (maxFrameCount_ >= 0 && currentFrameIndex_ + 1 >= maxFrameCount_)
    {
        isEnd_ = true;
        return cv::Mat();
    }

    cv::Mat frame;
    if (!capture_.read(frame) || frame.empty())
    {
        isEnd_ = true;
        return cv::Mat();
    }

    ++currentFrameIndex_;
    return frame;
}

cv::Size VideoFramesOStream::readFrameSize() const
{
    if (!capture_.isOpened())
        return cv::Size();

    return cv::Size(
        static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_WIDTH)),
        static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_HEIGHT))
    );
}
