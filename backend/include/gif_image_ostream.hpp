#pragma once

#include <filesystem> // std::filesystem::path

#include "image_ostream.hpp"

/** GIF 图像的图像帧数据流。 */
class GifImageOStream : public ImageOStream
{
public:
    explicit GifImageOStream(
        const std::filesystem::path& gifFilePath,
        long long maxn = -1, int maxw = -1, int maxh = -1) noexcept;
    ~GifImageOStream() override;

    bool      isOpened()   const override { return pixels_ != nullptr;    }
    bool      isEnd()      const override { return !isOpened() || isEnd_; }
    long long frameCount() const override;
    long long frameIndex() const override;

    /** 获取当前帧的播放延迟（单位为毫秒）。 */
    int delay()     const;
    /** 获取 GIF 循环播放次数（stb_image 不解析循环扩展块，恒返回无效值）。 */
    int loopCount() const;

protected:
    cv::Mat  readNextFrame()       override;
    cv::Size readFrameSize() const override;

private:
    bool      isEnd_           = false;
    long long maxn_            = INVALID_VALUE;
    long long frameIdx_        = INVALID_VALUE;
    long long totalFrameCount_ = INVALID_VALUE;

    int width_  = 0;
    int height_ = 0;
    unsigned char* pixels_ = nullptr;
    int*           delays_ = nullptr;
};
