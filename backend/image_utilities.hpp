#pragma once

#include <opencv2/core/mat.hpp>

/**
 * 将 GRAY 或 BGR 格式的图像转换为 BGRA 格式。
 *
 * @return 如果输入图像格式不合法则返回空图像。
 */
cv::Mat convertImageColorToBgra(const cv::Mat& image);

/**
 * 等比例限制给定图像的尺寸
 *
 * - 如果输入图像两个维度的尺寸均小于给定参数则不做任何处理。
 *
 * - 如果 \p maxWidth 和 \p maxHeight 存在且仅有一者小于 0，则仅处理大于 0 的维度。
 *
 * @return 如果输入图像为空或给定参数不合法则返回空图像。
 */
cv::Mat limitsImageSize(const cv::Mat& image, int maxWidth, int maxHeight);
