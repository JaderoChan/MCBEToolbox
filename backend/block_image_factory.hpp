#pragma once

#include <map>          // std::map
#include <memory>       // std::unique_ptr
#include <string>
#include <string_view>

#include <opencv2/core/mat.hpp>

#include "block.hpp"

// 实现类前置声明
class BlockImageFactoryPrivate;

/** 方块图生成工厂 */
class BlockImageFactory
{
public:
    /**
     * 任务进度回调函数。
     *
     * @param current    进度当前检查点索引
     * @param total      进度总检查点数量
     * @param blockImage 当前时刻的结果图像
     * @param stop       控制是否中止任务
     * @param userdata   传入给回调函数的用户自定义数据
     */
    using ProgressCallback = void (*)(
        std::size_t    current,
        std::size_t    total,
        const cv::Mat& blockImage,
        bool&          stop,
        void*          userdata);

    // 使用透明比较器以支持 string_view 的异构查找
    using BlockUsageCountType = std::map<std::string, std::size_t, std::less<>>;

    // 默认材质文件路径
    static constexpr const char* DEFAULT_TEXTURE_DIR_PATH = "./textures";

    BlockImageFactory();
    explicit BlockImageFactory(
        const BlockDataMap& blockDataMap,
        TargetSurface       targetSurface  = TargetSurface::Side,
        std::string_view    textureDirPath = DEFAULT_TEXTURE_DIR_PATH);
    ~BlockImageFactory();

    /** 设置可用的方块数据。 */
    void setBlockDataMap(const BlockDataMap& blockDataMap);

    /** 设置目标方块面。 */
    void setTargetSurface(TargetSurface targetSurface);

    /** 设置材质文件夹路径。 */
    void setTextureDirPath(std::string_view textureDirPath);

    /** 设置透明像素的替代方块，置空或传入指定面材质路径为空的方块则保留透明区域。 */
    void setFallbackBlock(const BlockDataPair* fallbackBlock = nullptr);

    /** 设置任务进度回调函数。 */
    void setProgressCallback(ProgressCallback callback = nullptr);

    /** 设置回调函数用户自定义数据。 */
    void setUserdata(void* userdata = nullptr);

    const BlockDataMap& getBlockDataMap() const;

    BlockDataMap& getBlockDataMapRef();

    /** 生成方块图，如果参数不合法或生成出错返回空 #cv::Mat。 */
    cv::Mat generateBlockImage(cv::Mat image);

    /** 获取方块用量信息。 */
    const BlockUsageCountType& getBlockUsageCount() const;

    /** 释放临时缓存数据。 */
    void releaseCaches();

private:
    std::unique_ptr<BlockImageFactoryPrivate> ptr_;
};
