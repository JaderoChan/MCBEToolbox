#include <image_utilities.hpp>

#include <stdio.h>   // fprintf
#include <algorithm> // std::min
#include <fstream>   // std::fstream

cv::Mat imreadUnicode(const std::filesystem::path& path, int flags)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
    {
        fprintf(stderr, "imreadUnicode() Failed to open the file '%s'\n", path.string().c_str());
        return cv::Mat();
    }

    const std::streamsize size = file.tellg();
    if (size <= 0)
        return cv::Mat();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> buf(static_cast<std::size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buf.data()), size))
    {
        fprintf(stderr, "imreadUnicode() Failed to read the file '%s'\n", path.string().c_str());
        return cv::Mat();
    }

    return cv::imdecode(buf, flags);
}

bool imwriteUnicode(const std::filesystem::path& path, const cv::Mat& image, const std::vector<int>& params)
{
    if (image.empty())
        return false;

    std::vector<unsigned char> buf;
    if (!cv::imencode(path.extension().string(), image, buf, params))
    {
        fprintf(stderr, "imwriteUnicode() Failed to encode the image for '%s'\n", path.string().c_str());
        return false;
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file)
    {
        fprintf(stderr, "imwriteUnicode() Failed to create the file '%s'\n", path.string().c_str());
        return false;
    }

    file.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(buf.size()));
    return file.good();
}

cv::Mat convertColorToBgra(const cv::Mat& image)
{
    cv::Mat ret;
    if (image.empty())
        return ret;

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
    if (image.empty() || size.empty()) return ret;
    if (image.size() == size)          return image;
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
