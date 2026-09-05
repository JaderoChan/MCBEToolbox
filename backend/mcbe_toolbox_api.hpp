#pragma once

#include <map>

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

/** 用于决定使用 #BlockData 哪一个面的数据 */
enum class TargetSurface
{
    Up,
    Down,
    Side
};

/**
 * 使用给定的 #BlockDataMap 将指定图像转换为方块图（使用方块材质作为像素组成的图片）。
 *
 * @param image           输入图像
 * @param blockDataMap    可用的“耗材”方块
 * @param targetSurface   指定使用的方块面
 * @param fallbackBlock   透明像素的替代方块，置空则保留透明区域
 * @param blockUsageCount 方块用量，格式为 {方块ID : 方块数量}
 * @param callback        回调函数
 * @param userdata        回调函数用户自定义数据
 *
 * @return 如果转换成功返回结果图像，否则返回空 #cv::Mat。
 */
cv::Mat convertImageToBlockImage(
    const cv::Mat&                           image,
    const BlockDataMap&                      blockDataMap,
    TargetSurface                            targetSurface,
    const std::pair<std::string, BlockData>* fallbackBlock   = nullptr,
    std::map<std::string, std::size_t>*      blockUsageCount = nullptr,
    ProgressCallback                         callback        = nullptr,
    void*                                    userdata        = nullptr);
