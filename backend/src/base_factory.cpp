#include <base_factory.hpp>

#include <stdexcept> // std::invalid_argument

#include "color_kd_tree.hpp"

BaseFactory::BaseFactory(const BlockDataMap& blocks, SurfaceDirection desiredSurface)
    : blocks_(blocks), desiredSurface_(desiredSurface), colorKdTree_(new ColorKdTree(blocks, desiredSurface))
{}

BaseFactory::~BaseFactory() = default;

void BaseFactory::setBlocks(const BlockDataMap& blocks)
{
    blocks_ = blocks;
    colorKdTree_->rebuild(blocks_, desiredSurface_);
}

void BaseFactory::setDesiredSurface(SurfaceDirection desiredSurface)
{
    desiredSurface_ = desiredSurface;
    colorKdTree_->rebuild(blocks_, desiredSurface_);
}

void BaseFactory::updateBlockUsageCount(BlockUsageMap& blockUsageCount, std::string_view id, std::size_t increment)
{
    auto it = blockUsageCount.find(id);
    if (it == blockUsageCount.end())
        blockUsageCount[std::string(id)] = increment;
    else
        ++(it->second);
}

bool BaseFactory::executeCallback(ProgressCallback callback, void* userdata, std::size_t current, std::size_t total)
{
    if (callback)
    {
        bool stop = false;
        callback(current, total, stop, userdata);
        return stop;
    }
    return false;
}

bool BaseFactory::isConfigured() const
{
    return !blocks_.empty() && colorKdTree_->isBuilt();
}

void BaseFactory::reset()
{
    blockUsageCount_.clear();
}

BlockDataPair BaseFactory::retrieveAppropriateBlock(const cv::Vec4b& color)
{
    // Alpha 通道值低于 128 的像素视为透明像素
    if (color[3] < 128)
    {
        if (fallbackBlock_)
            return *fallbackBlock_;
        return {"", nullptr};
    }
    else
    {
        const Rgb query{color[2], color[1], color[0]};
        return colorKdTree_->findNearest(query);
    }
}

std::array<int, 3> BaseFactory::compPosition(int x, int y, int z, int xs, int ys, int zs) const
{
    std::array<int, 3> ret;
    switch (desiredSurface_)
    {
        case SURFACE_DIRECTION_UP:
            ret[0] = xs - x - 1;
            ret[1] = zs - z - 1;
            ret[2] = ys - y - 1;
            break;
        case SURFACE_DIRECTION_BOTTOM:
            ret[0] = xs - x - 1;
            ret[1] = z;
            ret[2] = y;
            break;
        case SURFACE_DIRECTION_NORTH:
            ret[0] = xs - x - 1;
            ret[1] = ys - y - 1;
            ret[2] = z;
            break;
        case SURFACE_DIRECTION_SOUTH:
            ret[0] = x;
            ret[1] = ys - y - 1;
            ret[2] = zs - z - 1;
            break;
        case SURFACE_DIRECTION_EAST:
            ret[0] = z;
            ret[1] = ys - y - 1;
            ret[2] = xs - x - 1;
            break;
        case SURFACE_DIRECTION_WEST:
            ret[0] = zs - z - 1;
            ret[1] = ys - y - 1;
            ret[2] = x;
            break;
        default:
            throw std::invalid_argument("BaseFactory::compPosition(): invalid surface direction");
    }
    return ret;
}

void BaseFactory::updateBlockUsageCount(std::string_view id, std::size_t increment)
{
    updateBlockUsageCount(blockUsageCount_, id, increment);
}

bool BaseFactory::executeCallback(std::size_t current, std::size_t total)
{
    return executeCallback(callback_, userdata_, current, total);
}
