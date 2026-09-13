#pragma once

#include <assert.h> // assert
#include <vector>   // std::vector

#include "nanoflann.hpp" // nanoflann::*

#include <block.hpp>

struct RgbCloud
{
    std::vector<Rgb> pts;

    std::size_t kdtree_get_point_count() const
    {
        return pts.size();
    }

    float kdtree_get_pt(const std::size_t idx, const std::size_t dim) const
    {
        assert(idx < pts.size());
        assert(dim < 3);

        if (dim == 0) return pts[idx].r;
        if (dim == 1) return pts[idx].g;
        return pts[idx].b;
    }

    template<typename BBOX>
    bool kdtree_get_bbox(BBOX&) const
    {
        return false;
    }
};

// 方块颜色 KD 树（加速最近邻颜色方块数据的查找）
class ColorKdTree
{
public:
    template<typename Distance, class DatasetAdaptor, int32_t DIM = -1, typename index_t = uint32_t>
    using Adaptor       = nanoflann::KDTreeSingleIndexAdaptor<Distance, DatasetAdaptor, DIM, index_t>;
    using AdaptorParams = nanoflann::KDTreeSingleIndexAdaptorParams;
    using AdaptorFlags  = nanoflann::KDTreeSingleIndexAdaptorFlags;
    using Tree          = Adaptor<nanoflann::L2_Simple_Adaptor<float, RgbCloud>, RgbCloud, 3>;

    using BlockDataVector = std::vector<BlockDataPair>;

    ColorKdTree();
    ColorKdTree(const BlockDataMap& blocks, SurfaceDirection desiredSurface);

    // 更新参数并重新构建 KD 树。
    void rebuild(const BlockDataMap& blocks, SurfaceDirection desiredSurface);

    // 判断 KD 树是否已构建。
    bool isBuilt() const;

    // 寻找与给定颜色最近邻的方块数据。
    BlockDataPair findNearest(const Rgb& query) const;

private:
    bool            isBuilt_ = false;
    RgbCloud        cloud_;
    Tree            tree_;
    BlockDataVector blockDataVec_;
};
