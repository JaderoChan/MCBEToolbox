#include <video_image_ostream.hpp>

VideoImageOStream::VideoImageOStream(const std::string& videoFilePath, long long maxn, int maxw, int maxh) noexcept
    : VideoImageOStream(cv::VideoCapture(videoFilePath, DEFAULT_VIDEO_BACKEND), maxn, maxw, maxh)
{}

VideoImageOStream::VideoImageOStream(cv::VideoCapture capture, long long maxn, int maxw, int maxh) noexcept
    : ImageOStream(maxw, maxh), capture_(capture), maxn_(maxn)
{}

long long VideoImageOStream::frameCount() const
{
    if (!isOpened())
        return INVALID_VALUE;

    const double count = capture_.get(cv::CAP_PROP_FRAME_COUNT);
    const long long rawCount = count > 0 ? static_cast<long long>(count) : INVALID_VALUE;

    if (maxn_ < 0) return rawCount;
    return rawCount < maxn_ ? rawCount : maxn_;
}

long long VideoImageOStream::frameIndex() const
{
    return frameIdx_;
}

int VideoImageOStream::fps() const
{
    if (!isOpened())
        return INVALID_VALUE;
    return static_cast<int>(capture_.get(cv::CAP_PROP_FPS));
}

int VideoImageOStream::fourcc() const
{
    if (!isOpened())
        return INVALID_VALUE;
    return static_cast<int>(capture_.get(cv::CAP_PROP_FOURCC));
}

cv::Mat VideoImageOStream::readNextFrame()
{
    if (isEnd())
        return cv::Mat();

    if (maxn_ >= 0 && frameIdx_ + 1 >= maxn_)
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

    ++frameIdx_;
    return frame;
}

cv::Size VideoImageOStream::readFrameSize() const
{
    if (!isOpened())
        return cv::Size();

    return cv::Size(
        static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_WIDTH)),
        static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_HEIGHT))
    );
}
