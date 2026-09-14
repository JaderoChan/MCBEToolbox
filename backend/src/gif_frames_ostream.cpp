#include <gif_frames_ostream.hpp>

#include <gifdec.h>

#include <image_utilities.hpp>

GifFramesOStream::GifFramesOStream(const std::string& gifFilePath, long long maxn, int maxw, int maxh) noexcept
    : FramesOStream(maxw, maxh), maxn_(maxn), gif_(gd_open_gif(gifFilePath.c_str()))
{
    if (!gif_)
        return;

    // 预扫描 GIF 文件以获取总帧数，随后 rewind 回动画起始位置
    long long count = 0;
    while (gd_get_frame(gif_) == 1)
        ++count;
    gd_rewind(gif_);
    totalFrameCount_ = count;
}

GifFramesOStream::~GifFramesOStream()
{
    if (gif_)
        gd_close_gif(gif_);
}

long long GifFramesOStream::frameCount() const
{
    if (!isOpened())
        return INVALID_INDEX;

    if (maxn_ < 0) return totalFrameCount_;
    return totalFrameCount_ < maxn_ ? totalFrameCount_ : maxn_;
}

long long GifFramesOStream::frameIndex() const
{
    return frameIdx_;
}

int GifFramesOStream::delay() const
{
    if (!isOpened())
        return static_cast<int>(INVALID_INDEX);
    return gif_->gce.delay;
}

int GifFramesOStream::loopCount() const
{
    if (!isOpened())
        return static_cast<int>(INVALID_INDEX);
    return gif_->loop_count;
}

cv::Mat GifFramesOStream::readNextFrame()
{
    if (isEnd())
        return cv::Mat();

    if (maxn_ >= 0 && frameIdx_ + 1 >= maxn_)
    {
        isEnd_ = true;
        return cv::Mat();
    }

    if (gd_get_frame(gif_) != 1)
    {
        isEnd_ = true;
        return cv::Mat();
    }

    const int w = gif_->width;
    const int h = gif_->height;

    cv::Mat image(h, w, CV_8UC3);
    gd_render_frame(gif_, image.data);
    image = convertColorToBgra(image);
    if (image.empty())
        return cv::Mat();

    // 处理透明像素
    if (gif_->gce.transparency)
    {
        for (int y = 0; y < gif_->fh; ++y)
        {
            const int gy = gif_->fy + y;
            const uint8_t* indexRow = gif_->frame + static_cast<size_t>(gy) * w + gif_->fx;
            cv::Vec4b* pixelRow = image.ptr<cv::Vec4b>(gy) + gif_->fx;
            for (int x = 0; x < gif_->fw; ++x)
            {
                if (indexRow[x] == gif_->gce.tindex)
                    pixelRow[x][3] = 0;
            }
        }
    }

    ++frameIdx_;
    return image;
}

cv::Size GifFramesOStream::readFrameSize() const
{
    if (!isOpened())
        return cv::Size();
    return cv::Size(gif_->width, gif_->height);
}
