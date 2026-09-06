#pragma once

#include <map>
#include <memory>

#include <opencv2/opencv.hpp>
#include <mcnbt/mcnbt.hpp>

#include "block.hpp"

/** 用于决定使用 #BlockData 哪一个面的数据 */
enum class TargetSurface
{
    Up,
    Down,
    Side
};

/** 空气方块数据，可用于 fallbackBlock 参数 */
static inline const BlockData
FALLBACK_AIR_BLOCK_DATA{"minecraft:air", 0, BlockSurface()};
/** 空气方块数据对，可用于 fallbackBlock 参数 */
static inline const std::pair<std::string, BlockData>
FALLBACK_AIR_BLOCK{"minecraft:air", FALLBACK_AIR_BLOCK_DATA};

class BlockImageFactoryPrivate;
/** 生成方块图 */
class BlockImageFactory
{
public:
    /**
     * 进度回调函数。
     *
     * @param current    进度当前检查点索引
     * @param total      进度总检查点数量
     * @param blockImage 当前进度的结果
     * @param stop       控制是否中止任务
     */
    using ProgressCallback = void (*)(
        std::size_t    current,
        std::size_t    total,
        const cv::Mat& blockImage,
        bool&          stop,
        void*          userdata
    );

    BlockImageFactory();
    BlockImageFactory(
        const BlockDataMap& blockDataMap,
        TargetSurface targetSurface = TargetSurface::Side);
    ~BlockImageFactory();

    /** 设置可用的方块数据 */
    void setBlockDataMap(const BlockDataMap& blockDataMap);
    /** 设置目标方块面 */
    void setTargetSurface(TargetSurface targetSurface);
    /** 设置透明像素的替代方块，置空或传入指定面材质路径为空的方块则保留透明区域 */
    void setFallbackBlock(const std::pair<std::string, BlockData>* fallbackBlock = nullptr);
    /** 设置进度回调函数 */
    void setProgressCallback(ProgressCallback callback = nullptr);
    /** 设置进度回调函数用户自定义数据 */
    void setUserdata(void* userdata = nullptr);

    const BlockDataMap& getBlockDataMap() const;
    BlockDataMap& getBlockDataMapRef();

    /** 生成方块图，如果参数不合法或生成出错返回空 #cv::Mat */
    cv::Mat generateBlockImage(cv::Mat image);
    /** 获取方块用量信息 */
    const std::map<std::string, std::size_t>& getBlockUsageCount() const;

    /** 释放临时缓存数据 */
    void releaseCaches();

private:
    std::unique_ptr<BlockImageFactoryPrivate> ptr_;
};
