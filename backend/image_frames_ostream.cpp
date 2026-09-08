#include "image_frames_ostream.hpp"

#include <utility>

#include "image_utilities.hpp"

SingleImageFramesOStream::SingleImageFramesOStream(cv::Mat image)
    : image_(std::move(image))
{}

long long SingleImageFramesOStream::frameCount() const
{
    return isOpened() ? 1 : INVALID_INDEX;
}

long long SingleImageFramesOStream::currentFrameIndex() const
{
    return consumed_ ? 0 : INVALID_INDEX;
}

cv::Mat SingleImageFramesOStream::nextFrame()
{
    if (consumed_ || image_.empty())
        return cv::Mat();

    consumed_ = true;
    return image_;
}

cv::Size SingleImageFramesOStream::frameSize() const
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

VideoImageFramesOStream::VideoImageFramesOStream(const std::string& videoFilepath)
    : capture_(videoFilepath)
{}

VideoImageFramesOStream::VideoImageFramesOStream(int cameraIndex)
    : capture_(cameraIndex)
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

cv::Mat VideoImageFramesOStream::nextFrame()
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

cv::Size VideoImageFramesOStream::frameSize() const
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

LimitedSizeVideoImageFrameOStream::LimitedSizeVideoImageFrameOStream(
    const std::string& videoFilepath, const cv::Size& frameMaxSize)
    : VideoImageFramesOStream(videoFilepath), frameMaxSize_(frameMaxSize)
{}

LimitedSizeVideoImageFrameOStream::LimitedSizeVideoImageFrameOStream(
    int cameraIndex, const cv::Size& frameMaxSize)
    : VideoImageFramesOStream(cameraIndex), frameMaxSize_(frameMaxSize)
{}

cv::Mat LimitedSizeVideoImageFrameOStream::nextFrame()
{
    const cv::Mat& frame = VideoImageFramesOStream::nextFrame();
    return resizeImage(frame, frameSize());
}

cv::Size LimitedSizeVideoImageFrameOStream::frameSize() const
{
    if (frameSize_.empty())
    {
        frameSize_ = VideoImageFramesOStream::frameSize();
        frameSize_ = limitsSize(frameSize_, frameMaxSize_);
    }
    return frameSize_;
}

cv::Size LimitedSizeVideoImageFrameOStream::frameMaxSize() const
{
    return frameMaxSize_;
}

void LimitedSizeVideoImageFrameOStream::setFrameMaxSize(const cv::Size& frameMaxSize)
{
    frameMaxSize_ = frameMaxSize;
}
