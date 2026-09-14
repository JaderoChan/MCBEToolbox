#pragma once

#include <array>  // std::array
#include <string> // std::string
#include <map>    // std::map
#include <memory> // std::unique_ptr

#include <opencv2/core/matx.hpp> // cv::Vec

#include "block.hpp"

// 方块颜色 KD 树（加速最近邻颜色方块数据的查找）
class ColorKdTree;

class BaseFactory
{
public:
    /**
     * 任务进度回调函数。
     *
     * @param current  进度当前检查点索引
     * @param total    进度总检查点数量
     * @param stop     控制是否中止任务
     * @param userdata 传入给回调函数的用户自定义数据
     */
    using ProgressCallback = void (*)(std::size_t current, std::size_t total, bool& stop, void* userdata);
    /** {程序用方块 ID : 方块用量} */
    using BlockUsageMap    = std::map<std::string, std::size_t, std::less<>>;

    BaseFactory(const BlockDataMap& blocks, SurfaceDirection desiredSurface);
    virtual ~BaseFactory();

    void setBlocks(const BlockDataMap& blocks);
    void setDesiredSurface(SurfaceDirection desiredSurface);
    void setFallbackBlock(const BlockDataPair* fallbackBlock) { fallbackBlock_ = fallbackBlock; }
    void setProgressCallback(ProgressCallback callback)       { callback_      = callback;      }
    void setUserData(void* userdata)                          { userdata_      = userdata;      }

    const BlockDataMap&  getBlocks()           const          { return blocks_;                 }
    SurfaceDirection     getDesiredSurface()   const          { return desiredSurface_;         }
    const BlockDataPair* getFallbackBlock()    const          { return fallbackBlock_;          }
    ProgressCallback     getProgressCallback() const          { return callback_;               }
    void*                getUserData()         const          { return userdata_;               }

    const BlockUsageMap& getBlockUsageCount()  const          { return blockUsageCount_;        }

protected:
    static void updateBlockUsageCount(BlockUsageMap& blockUsageCount, std::string_view id, std::size_t increment);
    static bool executeCallback(ProgressCallback callback, void* userdata, std::size_t current, std::size_t total);

    virtual bool isConfigured() const;
    virtual void reset();

    // 根据给定颜色值查找最合适的方块数据，如果是透明像素则返回 fallbackBlock（若不存在则返回空无效 BlockDataPair）。
    BlockDataPair retrieveAppropriateBlock(const cv::Vec4b& color);
    std::array<int, 3> computePosition(int x, int y, int z, int xs, int ys, int zs) const;
    void updateBlockUsageCount(std::string_view id, std::size_t increment);
    bool executeCallback(std::size_t current, std::size_t total);

    BlockDataMap         blocks_;
    SurfaceDirection     desiredSurface_;
    const BlockDataPair* fallbackBlock_ = nullptr;
    ProgressCallback     callback_      = nullptr;
    void*                userdata_      = nullptr;

    BlockUsageMap blockUsageCount_;

    std::unique_ptr<ColorKdTree> colorKdTree_;
};
