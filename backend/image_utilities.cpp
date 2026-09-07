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

cv::Mat limitsImageSize(const cv::Mat& image, int maxWidth, int maxHeight)
{
    if (image.empty() || (maxWidth == 0 || maxHeight == 0) || (maxWidth < 0 && maxHeight < 0))
        return cv::Mat();

    if (image.cols < maxWidth && image.rows < maxHeight)
        return image;

    const double w = image.cols;
    const double h = image.rows;
    double ratio = 1.0;
    if (maxWidth > 0 && maxHeight > 0) ratio = std::min(maxWidth / w, maxHeight / h);
    else if (maxWidth > 0)             ratio = maxWidth  / w;
    else                               ratio = maxHeight / h;

    cv::Mat ret;
    cv::resize(image, ret, cv::Size(0, 0), ratio, ratio);
    return ret;
}
