#include <image_frames_ostream.hpp>

#include <opencv2/imgcodecs.hpp> // cv::imread

ImageFramesOStream::ImageFramesOStream(const std::string& imageFilePath, int maxw, int maxh) noexcept
    : ImageFramesOStream(cv::imread(imageFilePath, cv::IMREAD_UNCHANGED), maxw, maxh)
{}

ImageFramesOStream::ImageFramesOStream(cv::Mat image, int maxw, int maxh) noexcept
    : FramesOStream(maxw, maxh), image_(image)
{}

cv::Mat ImageFramesOStream::readNextFrame()
{
    if (isEnd())
        return cv::Mat();
    consumed_ = true;
    return image_;
}

cv::Size ImageFramesOStream::readFrameSize() const
{
    return image_.size();
}
