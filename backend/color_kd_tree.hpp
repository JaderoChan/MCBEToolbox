#pragma once

#include <memory>

#include "block.hpp"

// 实现类前置声明
class ColorKdTreePrivate;

/** 加速最近邻颜色方块数据的查找 */
class ColorKdTree
{
public:
    ColorKdTree();
    ColorKdTree(const BlockDataMap& blockDataMap, TargetSurface targetSurface);
    ~ColorKdTree();

    /** 更新参数并重新构建 KD 树 */
    void rebuild(const BlockDataMap& blockDataMap, TargetSurface targetSurface);
    /** 判断 KD 树是否以构建 */
    bool isBuilt() const;

    /** 寻找与给定颜色最近邻的方块数据 */
    BlockDataPair findNearest(const Rgb& query) const;

private:
    std::unique_ptr<ColorKdTreePrivate> ptr_;
};
