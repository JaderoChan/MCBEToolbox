#include "block_image_factory.hpp"

#include <unordered_map>

#include <opencv2/imgcodecs.hpp>

#include "image_utilities.hpp"
#include "color_kd_tree.hpp"

class BlockImageFactoryPrivate
{
public:
    using ProgressCallback    = BlockImageFactory::ProgressCallback;
    using BlockUsageCountType = BlockImageFactory::BlockUsageCountType;
    using TextureCacheType    = std::unordered_map<std::string, cv::Mat>;

    static constexpr size_t CALLBACK_GAP = 10000;

    BlockImageFactoryPrivate(
        const BlockDataMap& blockDataMap,
        TargetSurface       targetSurface,
        std::string_view    textureDirPath)
        : blockDataMap_(blockDataMap)
        , targetSurface_(targetSurface)
        , textureDirPath_(textureDirPath)
        , colorKdTree_(blockDataMap, targetSurface)
    {}

    void setBlockDataMap(const BlockDataMap& blockDataMap)
    {
        blockDataMap_ = blockDataMap;
        colorKdTree_.rebuild(blockDataMap_, targetSurface_);
    }

    void setTargetSurface(TargetSurface targetSurface)
    {
        targetSurface_ = targetSurface;
        colorKdTree_.rebuild(blockDataMap_, targetSurface_);
    }

    void setTextureDirPath(std::string_view textureDirPath)
    { textureDirPath_ = textureDirPath; }

    void setFallbackBlock(const BlockDataPair* fallbackBlock)
    { fallbackBlock_ = fallbackBlock; }

    void setProgressCallback(ProgressCallback callback = nullptr)
    { callback_ = callback; }

    void setUserdata(void* userdata)
    { userdata_ = userdata; }

    const BlockDataMap& getBlockDataMap() const
    { return blockDataMap_; }

    BlockDataMap& getBlockDataMapRef()
    { return blockDataMap_; }

    cv::Mat generateBlockImage(cv::Mat image);

    const BlockUsageCountType& getBlockUsageCount() const
    { return blockUsageCount_; }

    void releaseCaches()
    {
        blockUsageCount_.clear();
        cache_.clear();
    }

private:
    static std::string concatPath(const std::string& base, const std::string& path)
    { return base + "/" + path; }

    BlockDataMap         blockDataMap_;
    TargetSurface        targetSurface_;
    std::string          textureDirPath_;
    const BlockDataPair* fallbackBlock_  = nullptr;
    ProgressCallback     callback_       = nullptr;
    void*                userdata_       = nullptr;

    BlockUsageCountType  blockUsageCount_;
    TextureCacheType     cache_;

    // 通过 KD 树加速最近邻颜色查找
    ColorKdTree          colorKdTree_;
};

cv::Mat BlockImageFactoryPrivate::generateBlockImage(cv::Mat image)
{
    blockUsageCount_.clear();

    if (image.type() != CV_8UC1 && image.type() != CV_8UC3 && image.type() != CV_8UC4)
        return cv::Mat();
    image = convertImageColorToBgra(image);
    if (image.empty() || blockDataMap_.empty() || !colorKdTree_.isBuilt())
        return cv::Mat();

    // 回调函数参数
    std::size_t current = 0;
    const std::size_t total = image.rows * image.cols;

    // 假定所有材质图片尺寸为 16*16，所以每个像素对应 16*16 的方块材质区域
    cv::Mat ret(image.rows * 16, image.cols * 16, CV_8UC4, cv::Scalar(0.0, 0.0, 0.0, 0.0));
    for (int row = 0; row < image.rows; ++row)
    {
        for (int col = 0; col < image.cols; ++col)
        {
            // 遍历像素点颜色，并获取颜色与之最接近的方块数据
            // 如果是透明像素，根据 fallbackBlock 值决定是否跳过填充
            const auto rgba = image.at<cv::Vec4b>(row, col);
            std::string_view id;
            const BlockData* data = nullptr;
            // Alpha 通道值低于 128 的像素视为透明像素
            if (rgba[3] < 128)
            {
                if (fallbackBlock_)
                {
                    id   = fallbackBlock_->first;
                    data = fallbackBlock_->second;
                }
            }
            else
            {
                const Rgb rgb{rgba[2], rgba[1], rgba[0]};
                const auto& nearest = colorKdTree_.findNearest(rgb);
                id   = nearest.first;
                data = nearest.second;
            }

            if (data != nullptr)
            {
                const auto& surface = data->surface.get(targetSurface_);
                if (!surface.first.empty())
                {
                    // 如果当前材质还未被加载则将其加载至缓存中
                    const std::string texturePath = concatPath(textureDirPath_, surface.first);
                    if (cache_.find(texturePath) == cache_.end())
                    {
                        cv::Mat texture = cv::imread(texturePath, cv::IMREAD_UNCHANGED);
                        if (!texture.empty())
                            texture = convertImageColorToBgra(texture);
                        if (texture.empty())
                            texture = cv::Mat(16, 16, CV_8UC4, cv::Scalar(0.0, 0.0, 0.0, 0.0));
                        cache_[texturePath] = texture;
                    }

                    // 直接从缓存中加载方块材质
                    cv::Mat texture = cache_[texturePath];
                    // 复制方块材质至像素映射区域
                    texture.copyTo(ret(
                        cv::Range(row * 16, row * 16 + 16),
                        cv::Range(col * 16, col * 16 + 16)));
                }

                // 更新方块用量信息
                auto it = blockUsageCount_.find(id);
                if (it != blockUsageCount_.end())
                    ++(it->second);
                else
                    blockUsageCount_[std::string(id)] = 1;
            }

            // 回调函数
            ++current;
            if (callback_ && (current % CALLBACK_GAP == 0))
            {
                bool stop = false;
                callback_(current, total, ret, stop, userdata_);
                if (stop) return ret;
            }
        }
    }

    return ret;
}

BlockImageFactory::BlockImageFactory()
    : BlockImageFactory(BlockDataMap())
{}

BlockImageFactory::BlockImageFactory(
    const BlockDataMap& blockDataMap,
    TargetSurface       targetSurface,
    std::string_view    textureDirPath)
    : ptr_(new BlockImageFactoryPrivate(blockDataMap, targetSurface, textureDirPath))
{}

BlockImageFactory::~BlockImageFactory() = default;

void BlockImageFactory::setBlockDataMap(const BlockDataMap& blockDataMap)
{ ptr_->setBlockDataMap(blockDataMap); }

void BlockImageFactory::setTargetSurface(TargetSurface targetSurface)
{ ptr_->setTargetSurface(targetSurface); }

void BlockImageFactory::setTextureDirPath(std::string_view textureDirPath)
{ ptr_->setTextureDirPath(textureDirPath); }

void BlockImageFactory::setFallbackBlock(const BlockDataPair* fallbackBlock)
{ ptr_->setFallbackBlock(fallbackBlock); }

void BlockImageFactory::setProgressCallback(ProgressCallback callback)
{ ptr_->setProgressCallback(callback); }

void BlockImageFactory::setUserdata(void* userdata)
{ ptr_->setUserdata(userdata); }

const BlockDataMap& BlockImageFactory::getBlockDataMap() const
{ return ptr_->getBlockDataMap(); }

BlockDataMap& BlockImageFactory::getBlockDataMapRef()
{ return ptr_->getBlockDataMapRef(); }

cv::Mat BlockImageFactory::generateBlockImage(cv::Mat image)
{ return ptr_->generateBlockImage(image); }

const BlockImageFactory::BlockUsageCountType& BlockImageFactory::getBlockUsageCount() const
{ return ptr_->getBlockUsageCount(); }

void BlockImageFactory::releaseCaches()
{ ptr_->releaseCaches(); }
