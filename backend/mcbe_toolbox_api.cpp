#include "mcbe_toolbox_api.hpp"

#include <assert.h>
#include <stdexcept>
#include <unordered_map>

#include <nanoflann.hpp>

namespace
{

inline const std::pair<std::string, Rgb>&
getSurface(const BlockSurface& blockSurface, TargetSurface targetSurface)
{
    switch (targetSurface)
    {
        case TargetSurface::Up:   return blockSurface.up;
        case TargetSurface::Down: return blockSurface.down;
        case TargetSurface::Side: return blockSurface.side;
        default: throw std::invalid_argument("Invalid target surface");
    }
}

// 转换 8UC1 或 8UC3 类型的图像为 BGRA 格式。如果输入图像不是 8UC1/8UC3/8UC4 类型，输出图像将被置空。
void convertColorToBgra(cv::Mat src, cv::Mat& dst)
{
    switch (src.type())
    {
        case CV_8UC1: cv::cvtColor(src, dst, cv::COLOR_GRAY2BGRA); break;
        case CV_8UC3: cv::cvtColor(src, dst, cv::COLOR_BGR2BGRA);  break;
        case CV_8UC4: dst = src; break;
        default: dst = cv::Mat(); break;
    }
}

inline std::string concatPath(const std::string& path, const std::string& basePath = "./textures")
{
    return basePath + "/" + path;
}

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

struct ColorKdTree
{
    template<typename Distance, class DatasetAdaptor, int32_t DIM = -1, typename index_t = uint32_t>
    using Adaptor       = nanoflann::KDTreeSingleIndexAdaptor<Distance, DatasetAdaptor, DIM, index_t>;
    using AdaptorParams = nanoflann::KDTreeSingleIndexAdaptorParams;
    using AdaptorFlags  = nanoflann::KDTreeSingleIndexAdaptorFlags;
    using Tree          = Adaptor<nanoflann::L2_Simple_Adaptor<float, RgbCloud>, RgbCloud, 3>;

    using BlockDataPair   = std::pair<std::string_view, const BlockData*>;
    using BlockDataVector = std::vector<BlockDataPair>;

    RgbCloud        cloud;
    Tree            tree;
    BlockDataVector blockDataVec;

    ColorKdTree() : tree(3, cloud, AdaptorParams(10, AdaptorFlags::SkipInitialBuildIndex)) {}

    ColorKdTree(const BlockDataMap& blockDataMap, TargetSurface targetSurface)
        : ColorKdTree()
    {
        rebuild(blockDataMap, targetSurface);
    }

    void rebuild(const BlockDataMap& blockDataMap, TargetSurface targetSurface)
    {
        const std::size_t n = blockDataMap.size();
        cloud.pts.clear();
        blockDataVec.clear();
        cloud.pts.reserve(n);
        blockDataVec.reserve(n);

        for (const auto& [id, data] : blockDataMap)
        {
            // 过滤掉目标面为空的方块
            const auto& surface = getSurface(data->surface, targetSurface);
            if (!surface.first.empty())
            {
                cloud.pts.push_back(surface.second);
                blockDataVec.emplace_back(id, data);
            }
        }

        tree.buildIndex();
    }

    bool isValid() const
    {
        return
            cloud.kdtree_get_point_count() != 0 &&
            tree.size(tree)                != 0 &&
            blockDataVec.size()            != 0;
    }

    BlockDataPair findNearest(const Rgb& query) const
    {
        const float queryPt[3] = {
            static_cast<float>(query.r),
            static_cast<float>(query.g),
            static_cast<float>(query.b)
        };
        std::size_t retIdx;
        float outDistSq;
        nanoflann::KNNResultSet<float> resultSet(1);
        resultSet.init(&retIdx, &outDistSq);
        tree.findNeighbors(resultSet, queryPt);
        return blockDataVec[retIdx];
    }
};

} // namespace

class BlockImageFactoryPrivate
{
public:
    using ProgressCallback = BlockImageFactory::ProgressCallback;

    BlockImageFactoryPrivate() = default;

    BlockImageFactoryPrivate(const BlockDataMap& blockDataMap, TargetSurface targetSurface)
        : blockDataMap_(blockDataMap), targetSurface_(targetSurface),
        colorKdTree_(blockDataMap, targetSurface)
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

    void setFallbackBlock(const std::pair<std::string, BlockData>* fallbackBlock)
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

    const std::map<std::string, std::size_t>& getBlockUsageCount() const
    { return blockUsageCount_; }

    void releaseCaches()
    {
        blockUsageCount_.clear();
        cache_.clear();
    }

private:
    BlockDataMap                             blockDataMap_;
    TargetSurface                            targetSurface_ = TargetSurface::Side;
    const std::pair<std::string, BlockData>* fallbackBlock_ = nullptr;
    ProgressCallback                         callback_      = nullptr;
    void*                                    userdata_      = nullptr;

    std::map<std::string, std::size_t>       blockUsageCount_;
    std::unordered_map<std::string, cv::Mat> cache_;

    // 通过 KD 树加速最近邻颜色查找
    ColorKdTree                              colorKdTree_;
};

cv::Mat BlockImageFactoryPrivate::generateBlockImage(cv::Mat image)
{
    blockUsageCount_.clear();

    if (image.type() != CV_8UC1 && image.type() != CV_8UC3 && image.type() != CV_8UC4)
        return cv::Mat();
    convertColorToBgra(image, image);
    if (image.empty() || blockDataMap_.empty() || !colorKdTree_.isValid())
        return cv::Mat();

    // 用于回调函数
    // 每处理 10000 个像素执行一次回调函数
    constexpr std::size_t GAP = 10000;
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
            std::string id;
            const BlockData* data = nullptr;
            // Alpha 通道值低于 128 的像素视为透明像素
            if (rgba[3] < 128)
            {
                if (fallbackBlock_)
                {
                    id   = fallbackBlock_->first;
                    data = &fallbackBlock_->second;
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
                const auto& surface = getSurface(data->surface, targetSurface_);
                if (!surface.first.empty())
                {
                    // 如果当前材质还未被加载则将其加载至缓存中
                    const std::string texturePath = concatPath(surface.first);
                    if (cache_.find(texturePath) == cache_.end())
                    {
                        cv::Mat texture = cv::imread(texturePath, cv::IMREAD_UNCHANGED);
                        if (!texture.empty())
                            convertColorToBgra(texture, texture);
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
                ++(blockUsageCount_[id]);
            }

            // 回调函数
            ++current;
            if (callback_ && (current % GAP == 0))
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
    : ptr_(new BlockImageFactoryPrivate())
{}

BlockImageFactory::BlockImageFactory(const BlockDataMap& blockDataMap, TargetSurface targetSurface)
    : ptr_(new BlockImageFactoryPrivate(blockDataMap, targetSurface))
{}

BlockImageFactory::~BlockImageFactory() = default;

void BlockImageFactory::setBlockDataMap(const BlockDataMap& blockDataMap)
{ ptr_->setBlockDataMap(blockDataMap); }

void BlockImageFactory::setTargetSurface(TargetSurface targetSurface)
{ ptr_->setTargetSurface(targetSurface); }

void BlockImageFactory::setFallbackBlock(const std::pair<std::string, BlockData>* fallbackBlock)
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

const std::map<std::string, std::size_t>& BlockImageFactory::getBlockUsageCount() const
{ return ptr_->getBlockUsageCount(); }

void BlockImageFactory::releaseCaches()
{ ptr_->releaseCaches(); }
