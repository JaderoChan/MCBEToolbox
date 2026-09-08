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

cv::Mat resizeImage(const cv::Mat& image, const cv::Size& size)
{
    cv::Mat ret;
    cv::resize(image, ret, size);
    return ret;
}

cv::Size limitsSize(const cv::Size& size, int maxWidth, int maxHeight)
{
    if (size.empty() || (maxWidth == 0 || maxHeight == 0) || (maxWidth < 0 && maxHeight < 0))
        return cv::Size();

    if (size.width < maxWidth && size.height < maxHeight)
        return size;

    double ratio = 1.0;
    const double w = size.width;
    const double h = size.height;
    if (maxWidth > 0 && maxHeight > 0) ratio = std::min(maxWidth / w, maxHeight / h);
    else if (maxWidth > 0)             ratio = maxWidth  / w;
    else                               ratio = maxHeight / h;

    return cv::Size(size.width * ratio, size.height * ratio);
}

cv::Mat limitsImageSize(const cv::Mat& image, int maxWidth, int maxHeight)
{
    if (image.empty()) return cv::Mat();
    const cv::Size size = limitsSize(cv::Size(image.cols, image.rows), maxWidth, maxHeight);
    if (size.empty()) return cv::Mat();
    return resizeImage(image, size);
}
