#pragma once

#include <map>
#include <memory>
#include <vector>

#include <mcnbt/mcnbt.hpp>
#include <opencv2/core/mat.hpp>

#include "block.hpp"
#include "image_frames_ostream.hpp"

// 实现类前置声明
class StructureFactoryPrivate;

/**
 * 以给定图像/图像帧数据流生成结构文件
 *
 * - 在目标面为 #TargetSurface::Side 时。
 * 二维结构平面与地面的交线平行或等于 X 轴，以 X 轴正方向为图像 X 轴正方向，以 Y 轴正方向为图像 Y 轴正方向；
 * 三维结构体（多帧图像数据）帧平面与二维结构平面一致，以 Z 轴正方向为时间轴正方向。
 *
 * - 在目标面为 #TargetSurface::Up 时。
 * 二维结构平面平行或等于地面，以 X 轴正方向为图像 X 轴正方向，以 Z 轴正方向为图像 Y 轴正方向；
 * 三维结构体帧平面与二维结构平面一致，以 Y 轴负方向为时间轴正方向。
 *
 * - 在目标面为 #TargetSurface::Down 时。
 * 二维结构平面平行或等于地面，以 X 轴正方向为图像 X 轴正方向，以 Z 轴负方向为图像 Y 轴正方向；
 * 三维结构体帧平面与二维结构平面一致，以 Y 轴正方向为时间轴正方向。
 */
class StructureFactory
{
public:
    /**
     * 任务进度回调函数。
     *
     * @param current   进度当前检查点索引
     * @param total     进度总检查点数量
     * @param stop      控制是否中止任务
     * @param userdata  传入给回调函数的用户自定义数据
     */
    using ProgressCallback = void (*)(
        std::size_t current,
        std::size_t total,
        bool&       stop,
        void*       userdata
    );

    // 使用透明比较器以支持 string_view 的异构查找
    using BlockUsageCountType = std::map<std::string, std::size_t, std::less<>>;

    /** 默认构造函数，使用空 #BlockDataMap，#TargetSurface::Side 和 Version(1, 21, 50, 7) 为默认参数 */
    StructureFactory();
    StructureFactory(
        const BlockDataMap& blockDataMap,
        TargetSurface       targetSurface      = TargetSurface::Side,
        const Version&      blockFormatVersion = Version(1, 21, 50, 7));
    ~StructureFactory();

    /** 设置可用的方块数据 */
    void setBlockDataMap(const BlockDataMap& blockDataMap);
    /** 设置目标方块面和结构排布方式 */
    void setTargetSurface(TargetSurface targetSurface);
    /** 设置结构文件中方块格式的版本 */
    void setBlockFormatVersion(const Version& blockFormatVersion);
    /** 设置透明像素的替代方块，置空使用结构空位 */
    void setFallbackBlock(const BlockDataPair* fallbackBlock = nullptr);
    /** 设置任务进度回调函数 */
    void setProgressCallback(ProgressCallback callback = nullptr);
    /** 设置回调函数用户自定义数据 */
    void setUserdata(void* userdata = nullptr);

    const BlockDataMap& getBlockDataMap() const;
    BlockDataMap& getBlockDataMapRef();

    /** 生成图像结构文件，如果参数不合法返回空 #nbt::Tag。 */
    nbt::Tag generateSingleStructure(cv::Mat image);
    /** 生成视频结构文件（单个结构文件），如果参数不合法返回空 #nbt::Tag。 */
    nbt::Tag generateSingleStructure(ImageFramesOStream& stream);
    /**
     * 生成视频结构文件（每帧一个结构文件），如果参数不合法返回空数组。
     *
     * - 支持实时流
     * - 支持多线程处理
     * - 不支持回调函数
     */
    std::vector<nbt::Tag> generateDetachStructure(ImageFramesOStream& stream, int numThreads = 1);

    /** 获取方块用量信息 */
    const BlockUsageCountType& getBlockUsageCount() const;

    /** 释放临时缓存数据 */
    void releaseCaches();

private:
    std::unique_ptr<StructureFactoryPrivate> ptr_;
};
