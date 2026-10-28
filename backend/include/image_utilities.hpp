#pragma once

#include <filesystem> // std::filesystem::path
#include <vector>     // std::vector

#include <opencv2/core/mat.hpp>  // cv::Mat
#include <opencv2/imgcodecs.hpp> // cv::IMREAD_UNCHANGED
#include <opencv2/imgproc.hpp>   // cv::cvtColor, cv::resize

/**
 * 以 Unicode 安全的方式读取图像文件。
 *
 * @return 读取或解码失败时返回空图像。
 */
cv::Mat imreadUnicode(const std::filesystem::path& path, int flags = cv::IMREAD_UNCHANGED);

/**
 * 以 Unicode 安全的方式将写入图像。
 *
 * @return 是否成功。
 */
bool imwriteUnicode(
    const std::filesystem::path& path, const cv::Mat& image, const std::vector<int>& params = std::vector<int>());

/**
 * 将 GRAY 或 BGR 格式的图像转换为 BGRA 格式。
 *
 * @return 如果输入图像不合法返回空图像。
 */
cv::Mat convertColorToBgra(const cv::Mat& image);

/**
 * 将给定图像缩放为指定尺寸。
 *
 * @return 如果输入图像或尺寸不合法返回空图像。
 */
cv::Mat resizeImage(const cv::Mat& image, const cv::Size& size, int interpolation = cv::INTER_LINEAR);

/**
 * 等比例缩放给定尺寸，使其满足最大尺寸要求。
 *
 * @param maxw 最大宽度，设置为 -1 不做宽度限制
 * @param maxh 最大高度，设置为 -1 不做高度限制
 * @return 限制最大尺寸后的尺寸。
 *
 * @example
 * ([400, 200], 500, 500) -> [400, 200]
 * ([400, 200], 200, 200) -> [200, 100]
 * ([400, 200], 100, 100) -> [100, 25 ]
 * ([400, 200], -1,  100) -> [200, 100]
 * ([400, 200], 100, -1 ) -> [100, 25 ]
 * ([400, 200], 0,   0  ) -> [0,   0  ]
 * ([400, 200], -1,  -1 ) -> [400, 200] // 两个参数均为 -1 的话不做任何限制
 */
cv::Size limitSize(const cv::Size& size, int maxw, int maxh);

/**
 * 等比例缩放给定图像，使其满足最大尺寸要求。
 *
 * 如果输入图像尺寸已经满足最大尺寸要求，则不对图像做任何处理。
 *
 * @param maxw 最大宽度，设置为 -1 不做宽度限制
 * @param maxh 最大高度，设置为 -1 不做高度限制
 * @return 限制最大尺寸后的图像。
 *
 * @sa limitSize() resizeImage()
 */
cv::Mat limitImageSize(const cv::Mat& image, int maxw, int maxh);
