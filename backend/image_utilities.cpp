#include "image_utilities.hpp"

#include <algorithm>

#include <opencv2/imgproc.hpp>

cv::Mat convertImageColorToBgra(const cv::Mat& image)
{
    cv::Mat ret;
    switch (image.type())
    {
        case CV_8UC1: cv::cvtColor(image, ret, cv::COLOR_GRAY2BGRA); return ret;
        case CV_8UC3: cv::cvtColor(image, ret, cv::COLOR_BGR2BGRA);  return ret;
        case CV_8UC4: ret = image; return ret;
        default: return ret;
    }
}

cv::Size limitsSize(const cv::Size& size, const cv::Size& maxSize)
{
    if (size.empty() || (maxSize.width == 0 || maxSize.height == 0) ||
        (maxSize.width < 0 && maxSize.height < 0))
        return cv::Size();

    if (size.width < maxSize.width && size.height < maxSize.height)
        return size;

    double ratio = 0.0;
    if (maxSize.width > 0 && maxSize.height > 0)
        ratio = std::min(maxSize.width / size.width, maxSize.height / size.height);
    else if (maxSize.width > 0)
        ratio = maxSize.width  / size.width;
    else
        ratio = maxSize.height / size.height;

    return cv::Size(size.width * ratio, size.height * ratio);
}

cv::Size limitsSize(const cv::Size& size, int maxWidth, int maxHeight)
{
    return limitsSize(size, cv::Size(maxWidth, maxHeight));
}

cv::Mat resizeImage(const cv::Mat& image, const cv::Size& size)
{
    cv::Mat ret;
    cv::resize(image, ret, size);
    return ret;
}

cv::Mat limitsImageSize(const cv::Mat& image, const cv::Size& maxSize)
{
    if (image.empty()) return cv::Mat();
    const cv::Size size = limitsSize(cv::Size(image.cols, image.rows), maxSize);
    if (size.empty()) return cv::Mat();
    return resizeImage(image, size);
}

cv::Mat limitsImageSize(const cv::Mat& image, int maxWidth, int maxHeight)
{
    return limitsImageSize(image, cv::Size(maxWidth, maxHeight));
}
