#pragma once

#include <array>  // std::array
#include <string> // std::string
#include <map>    // std::map
#include <memory> // std::unique_ptr

#include <opencv2/core/matx.hpp> // cv::Vec

#include "block.hpp"

// 前置声明：方块颜色 KD 树（加速最近邻颜色方块数据的查找）。
class ColorKdTree;

/** 各类工厂类的基类，提供一些通用的接口。 */
class BaseFactory
{
public:
    // =================================================================================================================
    // > Type alias
    // =================================================================================================================

    /**
     * 任务进度回调函数。
     *
     * @param current  进度当前检查点索引
     * @param total    进度总检查点数量
     * @param stop     控制是否中止任务
     * @param userdata 传递给回调函数的用户自定义数据
     */
    using ProgressCallback = void (*)(std::size_t current, std::size_t total, bool& stop, void* userdata);

    /** 方块使用量数据类型 {程序用方块 ID : 方块用量} */
    using BlockUsageMap    = std::map<std::string, std::size_t, std::less<>>;

    // =================================================================================================================
    // > Const/Constexpr
    // =================================================================================================================

    // 并行生成任务时，挂起中的任务（已提交但结果尚未被消费）数量的滑动窗口大小相对于线程数的倍率，
    // 用于限制内存占用。实际窗口大小为 numThreads * TASK_WINDOW_SIZE_FACTOR。
    static constexpr std::size_t TASK_WINDOW_SIZE_FACTOR = 2;

    // =================================================================================================================
    // > Construct/Deconstruct
    // =================================================================================================================

    /**
     * @param blocks         用于各类任务的可用方块集合
     * @param desiredSurface 指定各类任务所使用的方块面及产物排布方式
     */
    BaseFactory(const BlockDataMap& blocks, SurfaceDirection desiredSurface);
    virtual ~BaseFactory();


    // =================================================================================================================
    // > Setter
    // =================================================================================================================

    void setBlocks(const BlockDataMap& blocks);
    void setDesiredSurface(SurfaceDirection desiredSurface);
    void setFallbackBlock(const BlockDataPair* fallbackBlock) { fallbackBlock_ = fallbackBlock; }
    void setProgressCallback(ProgressCallback callback)       { callback_      = callback;      }
    void setUserData(void* userdata)                          { userdata_      = userdata;      }

    // =================================================================================================================
    // > Getter
    // =================================================================================================================

    const BlockDataMap&  getBlocks()           const          { return blocks_;                 }
    SurfaceDirection     getDesiredSurface()   const          { return desiredSurface_;         }
    const BlockDataPair* getFallbackBlock()    const          { return fallbackBlock_;          }
    ProgressCallback     getProgressCallback() const          { return callback_;               }
    void*                getUserData()         const          { return userdata_;               }

    /** 获取上一个任务所使用的方块用量信息。 */
    const BlockUsageMap& getBlockUsageMap()    const          { return blockUsageMap_;          }

protected:
    // =================================================================================================================
    // > Convenience functions
    // =================================================================================================================

    // 便利函数：更新指定方块用量数据。
    static void updateBlockUsageMap(BlockUsageMap& blockUsageMap, std::string_view id, std::size_t increment);

    // 便利函数：执行指定回调函数，如果回调函数为 nullptr 则直接返回 false。
    // 返回 true 时意味着用户请求任务终止。
    static bool invokeProgressCallback(
        ProgressCallback callback, std::size_t current, std::size_t total, void* userdata);

    // 便利函数：更新实例的方块用量数据。
    void updateBlockUsageMap(std::string_view id, std::size_t increment);

    // 便利函数：调用实例的回调函数，如果回调函数为 nullptr 则直接返回 false。
    // 返回 true 时意味着用户请求任务终止。
    bool invokeProgressCallback(std::size_t current, std::size_t total);

    // =================================================================================================================
    // > Utilities
    // =================================================================================================================

    // 确保给定目录存在，如果不存在则创建，已存在但不是目录则失败。
    // 参数 logPrefix 用于打印日志时标记调用者来源。
    static bool ensureDirectoryExists(const std::string& dirPath, const char* logPrefix);

    // 判断当前工厂是否已成功配置（可用方块不为空，KD 树已构建等）。
    virtual bool isConfigured() const;

    // 重置任务独立的缓存数据/状态（每个任务开始前调用）。
    virtual void reinitializeState();

    // 根据给定颜色值查找最合适的方块数据，如果是透明像素则返回 fallbackBlock，
    // 若不存在则返回无效方块 BlockDataPair("", nullptr)。
    BlockDataPair retrieveAppropriateBlock(const cv::Vec4b& color);

    // 便利函数：计算坐标
    std::array<int, 3> computePosition(int x, int y, int z, int xs, int ys, int zs) const;

    // =================================================================================================================
    // > Member variables
    // =================================================================================================================

    BlockDataMap         blocks_;
    SurfaceDirection     desiredSurface_;
    const BlockDataPair* fallbackBlock_ = nullptr;
    ProgressCallback     callback_      = nullptr;
    void*                userdata_      = nullptr;

    BlockUsageMap blockUsageMap_;

    std::unique_ptr<ColorKdTree> colorKdTree_;
};
