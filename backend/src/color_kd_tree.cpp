#include "color_kd_tree.hpp"

ColorKdTree::ColorKdTree()
    : tree_(3, cloud_, AdaptorParams(10, AdaptorFlags::SkipInitialBuildIndex))
{}

ColorKdTree::ColorKdTree(const BlockDataMap& blocks, SurfaceDirection desiredSurface)
    : ColorKdTree()
{
    rebuild(blocks, desiredSurface);
}

void ColorKdTree::rebuild(const BlockDataMap& blocks, SurfaceDirection desiredSurface)
{
    const std::size_t n = blocks.size();
    cloud_.pts.clear();
    blockDataVec_.clear();
    cloud_.pts.reserve(n);
    blockDataVec_.reserve(n);

    for (const auto& [id, block] : blocks)
    {
        const auto& textureColor = block->surfaceData.get(desiredSurface);
        cloud_.pts.push_back(textureColor.second);
        blockDataVec_.emplace_back(id, block);
    }

    tree_.buildIndex();
    isBuilt_ = !blocks.empty();
}

bool ColorKdTree::isBuilt() const
{
    return isBuilt_;
}

BlockDataPair ColorKdTree::findNearest(const Rgb& query) const
{
    assert(isBuilt_);

    const float rgbPt[3] = {
        static_cast<float>(query.r),
        static_cast<float>(query.g),
        static_cast<float>(query.b)
    };

    std::size_t retIdx;
    float outDistSq;
    nanoflann::KNNResultSet<float> resultSet(1);
    resultSet.init(&retIdx, &outDistSq);
    tree_.findNeighbors(resultSet, rgbPt);

    return blockDataVec_[retIdx];
}
