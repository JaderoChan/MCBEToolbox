#include <gif_image_ostream.hpp>

#include <fstream> // std::ifstream
#include <vector>  // std::vector

#include <opencv2/imgproc.hpp> // cv::cvtColor

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GifImageOStream::GifImageOStream(const std::filesystem::path& gifFilePath, long long maxn, int maxw, int maxh) noexcept
    : ImageOStream(maxw, maxh), maxn_(maxn)
{
    std::ifstream file(gifFilePath, std::ios::binary | std::ios::ate);
    if (!file)
        return;

    const std::streamsize size = file.tellg();
    if (size <= 0)
        return;
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> buf(static_cast<std::size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buf.data()), size))
        return;

    int frameCount = 0;
    pixels_ = stbi_load_gif_from_memory(
        buf.data(), static_cast<int>(buf.size()), &delays_, &width_, &height_, &frameCount, nullptr, 4);
    if (pixels_)
        totalFrameCount_ = frameCount;
}

GifImageOStream::~GifImageOStream()
{
    if (pixels_) stbi_image_free(pixels_);
    if (delays_) stbi_image_free(delays_);
}

long long GifImageOStream::frameCount() const
{
    if (!isOpened())
        return INVALID_VALUE;

    if (maxn_ < 0) return totalFrameCount_;
    return totalFrameCount_ < maxn_ ? totalFrameCount_ : maxn_;
}

long long GifImageOStream::frameIndex() const
{
    return frameIdx_;
}

int GifImageOStream::delay() const
{
    if (!isOpened() || frameIdx_ < 0)
        return static_cast<int>(INVALID_VALUE);
    return delays_[frameIdx_];
}

int GifImageOStream::loopCount() const
{
    return static_cast<int>(INVALID_VALUE);
}

cv::Mat GifImageOStream::readNextFrame()
{
    if (isEnd())
        return cv::Mat();

    if (maxn_ >= 0 && frameIdx_ + 1 >= maxn_)
    {
        isEnd_ = true;
        return cv::Mat();
    }

    const long long nextIdx = frameIdx_ + 1;
    if (nextIdx >= totalFrameCount_)
    {
        isEnd_ = true;
        return cv::Mat();
    }

    const std::size_t frameBytes = static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_) * 4;
    const cv::Mat rgba(height_, width_, CV_8UC4, pixels_ + static_cast<std::size_t>(nextIdx) * frameBytes);

    cv::Mat bgra;
    cv::cvtColor(rgba, bgra, cv::COLOR_RGBA2BGRA);

    frameIdx_ = nextIdx;
    return bgra;
}

cv::Size GifImageOStream::readFrameSize() const
{
    if (!isOpened())
        return cv::Size();
    return cv::Size(width_, height_);
}
