#pragma once

#include <string> // std::string

#include <opencv2/videoio.hpp> // cv::VideoCapture

#include "image_ostream.hpp"

#ifdef _WIN32
    #define DEFAULT_VIDEO_BACKEND cv::CAP_MSMF
#elif defined(__APPLE__)
    #define DEFAULT_VIDEO_BACKEND cv::CAP_AVFOUNDATION
#else
    #define DEFAULT_VIDEO_BACKEND cv::CAP_FFMPEG
#endif

/** 视频的图像帧数据流。 */
class VideoImageOStream : public ImageOStream
{
public:
    explicit VideoImageOStream(
        const std::string& videoFilePath,
        long long maxn = -1, int maxw = -1, int maxh = -1) noexcept;
    explicit VideoImageOStream(
        cv::VideoCapture capture,
        long long maxn = -1, int maxw = -1, int maxh = -1) noexcept;

    bool      isOpened()   const override { return capture_.isOpened();            }
    bool      isEnd()      const override { return !capture_.isOpened() || isEnd_; }
    long long frameCount() const override;
    long long frameIndex() const override;

    /** 获取视频的帧数。 */
    int fps()    const;
    /** 获取视频的编码格式。 */
    int fourcc() const;

protected:
    cv::Mat  readNextFrame()       override;
    cv::Size readFrameSize() const override;

private:
    bool      isEnd_    = false;
    long long maxn_     = INVALID_VALUE;
    long long frameIdx_ = INVALID_VALUE;
    cv::VideoCapture capture_;
};
