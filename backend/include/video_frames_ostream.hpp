#pragma once

#include <string> // std::string

#include <opencv2/videoio.hpp> // cv::VideoCapture

#include "frames_ostream.hpp"

#ifdef _WIN32
    #define DEFAULT_VIDEO_BACKEND cv::CAP_MSMF
#elif defined(__APPLE__)
    #define DEFAULT_VIDEO_BACKEND cv::CAP_AVFOUNDATION
#else
    #define DEFAULT_VIDEO_BACKEND cv::CAP_FFMPEG
#endif

/** 视频的图像帧数据流。 */
class VideoFramesOStream : public FramesOStream
{
public:
    explicit VideoFramesOStream(
        const std::string& videoFilePath,
        long long maxn = -1, int maxw = -1, int maxh = -1) noexcept;
    explicit VideoFramesOStream(
        cv::VideoCapture capture,
        long long maxn = -1, int maxw = -1, int maxh = -1) noexcept;

    bool      isOpened()   const override { return capture_.isOpened();            }
    bool      isEnd()      const override { return !capture_.isOpened() || isEnd_; }
    long long frameCount() const override;
    long long frameIndex() const override;

    int fps()    const;
    int fourcc() const;

protected:
    cv::Mat  readNextFrame() override;
    cv::Size readFrameSize() const override;

private:
    bool      isEnd_    = false;
    long long maxn_     = INVALID_INDEX;
    long long frameIdx_ = INVALID_INDEX;
    cv::VideoCapture capture_;
};
