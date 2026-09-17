#include <single_image_ostream.hpp>

#include <opencv2/imgcodecs.hpp> // cv::imread

SingleImageOStream::SingleImageOStream(const std::string& imageFilePath, int maxw, int maxh) noexcept
    : SingleImageOStream(cv::imread(imageFilePath, cv::IMREAD_UNCHANGED), maxw, maxh)
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
