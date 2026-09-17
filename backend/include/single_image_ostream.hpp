#pragma once

#include <string> // std::string

#include "image_ostream.hpp"

/** 单张图像的图像帧数据流（总帧数固定为 1）。 */
class SingleImageOStream : public ImageOStream
{
public:
    explicit SingleImageOStream(const std::string& imageFilePath, int maxw = -1, int maxh = -1) noexcept;
    explicit SingleImageOStream(cv::Mat image,                    int maxw = -1, int maxh = -1) noexcept;

    bool      isOpened()   const override { return !image_.empty();                }
    bool      isEnd()      const override { return consumed_ || image_.empty();    }
    long long frameCount() const override { return isOpened() ? 1 : INVALID_VALUE; }
    long long frameIndex() const override { return consumed_  ? 0 : INVALID_VALUE; }

protected:
    cv::Mat  readNextFrame()       override;
    cv::Size readFrameSize() const override;

private:
    bool    consumed_ = false;
    cv::Mat image_;
};
