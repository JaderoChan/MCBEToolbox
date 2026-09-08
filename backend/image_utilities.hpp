#pragma once

#include <opencv2/core/mat.hpp>

/**
 * 将 GRAY 或 BGR 格式的图像转换为 BGRA 格式。
 *
 * @return 如果输入图像格式不合法则返回空图像。
 */
cv::Mat convertImageColorToBgra(const cv::Mat& image);

cv::Mat resizeImage(const cv::Mat& image, const cv::Size& size);

/**
 * 等比例缩放给定尺寸，使其满足最大尺寸要求.
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
cv::Size limitsSize(const cv::Size& size, int maxWidth, int maxHeight);

/**
 * 等比例缩放图像，使其满足最大尺寸要求。
 *
 * - 如果输入图像两个维度的尺寸均小于给定参数则不做任何处理。
 * - 将指定维度置为 -1 则不对指定维度进行限制，如果两个维度最大值均为 -1 则返回未经限制的原图。
 *
 * @return 如果输入图像为空或给定参数不合法则返回空图像。
 * @sa limitsSize()
 */
cv::Mat limitsImageSize(const cv::Mat& image, int maxWidth, int maxHeight);

