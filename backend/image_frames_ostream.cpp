#include "image_frames_ostream.hpp"

#include <utility>

#include "image_utilities.hpp"

ImageFramesOStream::ImageFramesOStream(int maxFrameWidth, int maxFrameHeight)
    : maxFrameWidth_(maxFrameWidth), maxFrameHeight_(maxFrameHeight)
{}

int ImageFramesOStream::maxFrameWidth() const
{
    return maxFrameWidth_;
}

int ImageFramesOStream::maxFrameHeight() const
{
    return maxFrameHeight_;
}

void ImageFramesOStream::setMaxFrameWidth(int maxFrameWidth)
{
    maxFrameWidth_    = maxFrameWidth;
    cachedFrameSize_  = cv::Size();
}

void ImageFramesOStream::setMaxFrameHeight(int maxFrameHeight)
{
    maxFrameHeight_   = maxFrameHeight;
    cachedFrameSize_  = cv::Size();
}

void ImageFramesOStream::setMaxFrameSize(int maxFrameWidth, int maxFrameHeight)
{
    maxFrameWidth_    = maxFrameWidth;
    maxFrameHeight_   = maxFrameHeight;
    cachedFrameSize_  = cv::Size();
}

cv::Mat ImageFramesOStream::nextFrame()
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

cv::Size ImageFramesOStream::frameSize() const
{
    if (cachedFrameSize_.empty())
    {
        const cv::Size rawSize = readFrameSize();
        if (!rawSize.empty())
            cachedFrameSize_ = limitsSize(rawSize, maxFrameWidth_, maxFrameHeight_);
    }
    return cachedFrameSize_;
}

SingleImageFramesOStream::SingleImageFramesOStream(cv::Mat image, int maxFrameWidth, int maxFrameHeight)
    : ImageFramesOStream(maxFrameWidth, maxFrameHeight), image_(std::move(image))
{}

long long SingleImageFramesOStream::frameCount() const
{
    return isOpened() ? 1 : INVALID_INDEX;
}

long long SingleImageFramesOStream::currentFrameIndex() const
{
    return consumed_ ? 0 : INVALID_INDEX;
}

cv::Mat SingleImageFramesOStream::readNextFrame()
{
    if (consumed_ || image_.empty())
        return cv::Mat();

    consumed_ = true;
    return image_;
}

cv::Size SingleImageFramesOStream::readFrameSize() const
{
    return image_.size();
}

bool SingleImageFramesOStream::isEnd() const
{
    return consumed_ || image_.empty();
}

bool SingleImageFramesOStream::isOpened() const
{
    return !image_.empty();
}

VideoImageFramesOStream::VideoImageFramesOStream(
    const std::string& videoFilepath, int maxFrameWidth, int maxFrameHeight)
    : ImageFramesOStream(maxFrameWidth, maxFrameHeight), capture_(videoFilepath)
{}

VideoImageFramesOStream::VideoImageFramesOStream(cv::VideoCapture capture, int maxFrameWidth, int maxFrameHeight)
    : ImageFramesOStream(maxFrameWidth, maxFrameHeight), capture_(std::move(capture))
{}

long long VideoImageFramesOStream::frameCount() const
{
    if (!capture_.isOpened())
        return INVALID_INDEX;

    const double count = capture_.get(cv::CAP_PROP_FRAME_COUNT);
    return count > 0 ? static_cast<long long>(count) : INVALID_INDEX;
}

long long VideoImageFramesOStream::currentFrameIndex() const
{
    return currentFrameIndex_;
}

cv::Mat VideoImageFramesOStream::readNextFrame()
{
    if (!capture_.isOpened() || isEnd_)
        return cv::Mat();

    cv::Mat frame;
    if (!capture_.read(frame) || frame.empty())
    {
        isEnd_ = true;
        return cv::Mat();
    }

    ++currentFrameIndex_;
    return frame;
}

cv::Size VideoImageFramesOStream::readFrameSize() const
{
    if (!capture_.isOpened())
        return cv::Size();

    return cv::Size(
        static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_WIDTH)),
        static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_HEIGHT))
    );
}

bool VideoImageFramesOStream::isEnd() const
{
    return isEnd_ || !capture_.isOpened();
}

bool VideoImageFramesOStream::isOpened() const
{
    return capture_.isOpened();
}

RealTimeVideoImageFramesOStream::RealTimeVideoImageFramesOStream(
    int cameraIndex, int maxFrameWidth, int maxFrameHeight)
    : VideoImageFramesOStream(cv::VideoCapture(cameraIndex), maxFrameWidth, maxFrameHeight)
{}

long long RealTimeVideoImageFramesOStream::frameCount() const
{
    return INVALID_INDEX;
}
