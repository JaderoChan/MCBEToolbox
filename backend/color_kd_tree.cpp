#include "color_kd_tree.hpp"

#include <assert.h>
#include <vector>

#include <nanoflann.hpp>

namespace
{

struct RgbCloud
{
    std::vector<Rgb> pts;

    std::size_t kdtree_get_point_count() const { return pts.size(); }

    float kdtree_get_pt(const std::size_t idx, const std::size_t dim) const
    {
        assert(dim >= 0 && dim <= 3);

        if (dim == 0) return pts[idx].r;
        if (dim == 1) return pts[idx].g;
        return pts[idx].b;
    }

    template<class BBOX>
    bool kdtree_get_bbox(BBOX&) const { return false; }
};

}

class ColorKdTreePrivate
{
public:
    template<typename Distance, class DatasetAdaptor, int32_t DIM = -1, typename index_t = uint32_t>
    using Adaptor       = nanoflann::KDTreeSingleIndexAdaptor<Distance, DatasetAdaptor, DIM, index_t>;
    using AdaptorParams = nanoflann::KDTreeSingleIndexAdaptorParams;
    using AdaptorFlags  = nanoflann::KDTreeSingleIndexAdaptorFlags;
    using Tree          = Adaptor<nanoflann::L2_Simple_Adaptor<float, RgbCloud>, RgbCloud, 3>;

    using BlockDataVector = std::vector<BlockDataPair>;

    ColorKdTreePrivate();
    ColorKdTreePrivate(const BlockDataMap& blockDataMap, TargetSurface targetSurface);

    void rebuild(const BlockDataMap& blockDataMap, TargetSurface targetSurface);
    bool isBuilt();

    BlockDataPair findNearest(const Rgb& query) const;

private:
    bool            isBuilt_ = false;
    RgbCloud        cloud_;
    Tree            tree_;
    BlockDataVector blockDataVec_;
};

ColorKdTreePrivate::ColorKdTreePrivate()
    : tree_(3, cloud_, AdaptorParams(10, AdaptorFlags::SkipInitialBuildIndex)) {}

ColorKdTreePrivate::ColorKdTreePrivate(const BlockDataMap& blockDataMap, TargetSurface targetSurface)
    : ColorKdTreePrivate() { rebuild(blockDataMap, targetSurface); }

void ColorKdTreePrivate::rebuild(const BlockDataMap& blockDataMap, TargetSurface targetSurface)
{
    const std::size_t n = blockDataMap.size();
    cloud_.pts.clear();
    blockDataVec_.clear();
    cloud_.pts.reserve(n);
    blockDataVec_.reserve(n);

    for (const auto& [id, data] : blockDataMap)
    {
        const auto& surface = data->surface.get(targetSurface);
        cloud_.pts.push_back(surface.second);
        blockDataVec_.emplace_back(id, data);
    }

    tree_.buildIndex();
    isBuilt_ = !blockDataMap.empty();
}

bool ColorKdTreePrivate::isBuilt()
{
    return isBuilt_;
}

BlockDataPair ColorKdTreePrivate::findNearest(const Rgb& query) const
{
    assert(isBuilt_);

    const float queryPt[3] = {
        static_cast<float>(query.r),
        static_cast<float>(query.g),
        static_cast<float>(query.b)
    };

    std::size_t retIdx;
    float outDistSq;
    nanoflann::KNNResultSet<float> resultSet(1);
    resultSet.init(&retIdx, &outDistSq);
    tree_.findNeighbors(resultSet, queryPt);

    return blockDataVec_[retIdx];
}

ColorKdTree::ColorKdTree()
    : ptr_(new ColorKdTreePrivate())
{}

ColorKdTree::ColorKdTree(const BlockDataMap& blockDataMap, TargetSurface targetSurface)
    : ptr_(new ColorKdTreePrivate(blockDataMap, targetSurface))
{}

ColorKdTree::~ColorKdTree() = default;

void ColorKdTree::rebuild(const BlockDataMap& blockDataMap, TargetSurface targetSurface)
{ ptr_->rebuild(blockDataMap, targetSurface); }

bool ColorKdTree::isBuilt() const
{ return ptr_->isBuilt(); }

BlockDataPair ColorKdTree::findNearest(const Rgb& query) const
{ return ptr_->findNearest(query); }
