#pragma once

#include <string> // std::string

#include "image_ostream.hpp"

struct gd_GIF;

/** GIF 图像的图像帧数据流。 */
class GifImageOStream : public ImageOStream
{
public:
    explicit GifImageOStream(
        const std::string& gifFilePath,
        long long maxn = -1, int maxw = -1, int maxh = -1) noexcept;
    ~GifImageOStream() override;

    bool      isOpened()   const override { return gif_ != nullptr;       }
    bool      isEnd()      const override { return !isOpened() || isEnd_; }
    long long frameCount() const override;
    long long frameIndex() const override;

    /** 获取当前帧的播放延迟（单位为 0.01 秒）。 */
    int delay()     const;
    /** 获取 GIF 循环播放次数（0 表示无限循环）。 */
    int loopCount() const;

protected:
    cv::Mat  readNextFrame()       override;
    cv::Size readFrameSize() const override;

private:
    bool      isEnd_           = false;
    long long maxn_            = INVALID_VALUE;
    long long frameIdx_        = INVALID_VALUE;
    long long totalFrameCount_ = INVALID_VALUE;
    gd_GIF*   gif_             = nullptr;
};
