#pragma once

#include <opencv2/opencv.hpp>

#include "block.hpp"

/**
 * 用于各类耗时任务的进度回调函数。
 *
 * @param current 进度当前检查点索引
 * @param total   进度总检查点数量
 * @param stop    控制是否中止任务
 */
using ProgressCallback = void (*)(
    std::size_t current,
    std::size_t total,
    bool&       stop,
    void*       userdata
);

/**
 * 使用给定的 BlockDataMap 将指定图像转换为方块图（使用方块材质作为像素组成的图片）。
 *
 * 使用 up 面（未来可能进行更改）
 *
 * @param image           8UC4（BGRA8888）颜色格式的非空图像
 * @param blockDataMap    可用的“耗材”方块
 * @param fallbackBlock   透明像素的替代方块，置空则保留透明区域
 * @param blockUsageCount 方块用量，格式为 {方块ID : 方块数量}
 * @param callback        回调函数
 * @param userdata        回调函数用户自定义数据
 */
cv::Mat convertImageToBlockImage(
    const cv::Mat&                                image,
    const BlockDataMap&                           blockDataMap,
    const std::pair<std::string, BlockData>*      fallbackBlock   = nullptr,
    std::unordered_map<std::string, std::size_t>* blockUsageCount = nullptr,
    ProgressCallback                              callback        = nullptr,
    void*                                         userdata        = nullptr);
