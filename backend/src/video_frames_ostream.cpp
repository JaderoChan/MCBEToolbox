#include <video_frames_ostream.hpp>

VideoFramesOStream::VideoFramesOStream(const std::string& videoFilePath, long long maxn, int maxw, int maxh) noexcept
    : VideoFramesOStream(cv::VideoCapture(videoFilePath, DEFAULT_VIDEO_BACKEND), maxn, maxw, maxh)
{}

VideoFramesOStream::VideoFramesOStream(cv::VideoCapture capture, long long maxn, int maxw, int maxh) noexcept
    : FramesOStream(maxw, maxh), capture_(capture), maxn_(maxn)
{}

long long VideoFramesOStream::frameCount() const
{
    if (!isOpened())
        return INVALID_INDEX;

    const double count = capture_.get(cv::CAP_PROP_FRAME_COUNT);
    const long long rawCount = count > 0 ? static_cast<long long>(count) : INVALID_INDEX;

    if (maxn_ < 0) return rawCount;
    return rawCount < maxn_ ? rawCount : maxn_;
}

long long VideoFramesOStream::frameIndex() const
{
    return frameIdx_;
}

int VideoFramesOStream::fps() const
{
    if (!isOpened())
        return INVALID_INDEX;
    return static_cast<int>(capture_.get(cv::CAP_PROP_FPS));
}

int VideoFramesOStream::fourcc() const
{
    if (!isOpened())
        return INVALID_INDEX;
    return static_cast<int>(capture_.get(cv::CAP_PROP_FOURCC));
}

cv::Mat VideoFramesOStream::readNextFrame()
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

cv::Size VideoFramesOStream::readFrameSize() const
{
    if (!isOpened())
        return cv::Size();

    return cv::Size(
        static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_WIDTH)),
        static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_HEIGHT))
    );
}
