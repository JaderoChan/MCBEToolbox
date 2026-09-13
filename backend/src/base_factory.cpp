#include <base_factory.hpp>

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

void BaseFactory::updateBlockUsageCount(std::string_view id, std::size_t increment)
{
    updateBlockUsageCount(blockUsageCount_, id, increment);
}

bool BaseFactory::executeCallback(std::size_t current, std::size_t total)
{
    return executeCallback(callback_, userdata_, current, total);
}
