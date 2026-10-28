#include <single_image_ostream.hpp>

#include <image_utilities.hpp>

SingleImageOStream::SingleImageOStream(const std::filesystem::path& imageFilePath, int maxw, int maxh) noexcept
    : SingleImageOStream(imreadUnicode(imageFilePath), maxw, maxh)
{}

SingleImageOStream::SingleImageOStream(cv::Mat image, int maxw, int maxh) noexcept
    : ImageOStream(maxw, maxh), image_(image)
{}

cv::Mat SingleImageOStream::readNextFrame()
{
    if (isEnd())
        return cv::Mat();
    consumed_ = true;
    return image_;
}

cv::Size SingleImageOStream::readFrameSize() const
{
    return image_.size();
}
