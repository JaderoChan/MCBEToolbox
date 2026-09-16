#include <image_utilities.hpp>

#include <stdio.h>   // fprintf
#include <algorithm> // std::min

cv::Mat convertColorToBgra(const cv::Mat& image)
{
    cv::Mat ret;
    switch (image.type())
    {
        case CV_8UC1: cv::cvtColor(image, ret, cv::COLOR_GRAY2BGRA); return ret;
        case CV_8UC3: cv::cvtColor(image, ret, cv::COLOR_BGR2BGRA);  return ret;
        case CV_8UC4: ret = image; return ret;
        default:
            fprintf(
                stderr,
                "convertColorToBgra() The type (%d) of image be passed is not support\n",
                image.type()
            );
            return ret;
    }
}

cv::Mat resizeImage(const cv::Mat& image, const cv::Size& size, int interpolation)
{
    cv::Mat ret;
    if (size.empty()) return ret;
    cv::resize(image, ret, size, 0.0, 0.0, interpolation);
    return ret;
}

cv::Size limitSize(const cv::Size& size, int maxw, int maxh)
{
    if (size.empty() || (maxw == 0 || maxh == 0))
        return cv::Size();

    if ((size.width < maxw && size.height < maxh) || (maxw < 0 && maxh < 0))
        return size;

    double ratio = 1.0;
    const double w = size.width;
    const double h = size.height;
    if (maxw > 0 && maxh > 0) ratio = std::min(maxw / w, maxh / h);
    else if (maxw > 0)        ratio = maxw / w;
    else                      ratio = maxh / h;

    return cv::Size(size.width * ratio, size.height * ratio);
}

cv::Mat limitImageSize(const cv::Mat& image, int maxw, int maxh)
{
    if (image.empty())        return cv::Mat();
    if (maxw < 0 && maxh < 0) return image;
    const cv::Size size = limitSize(cv::Size(image.cols, image.rows), maxw, maxh);
    return resizeImage(image, size);
}
