#include "structure_factory.hpp"

#include <unordered_map>

#include <mcnbt/be/mcstructure.hpp>

#include "color_kd_tree.hpp"
#include "image_utilities.hpp"
#include "thread_pool.hpp"

class StructureFactoryPrivate
{
public:
    using ProgressCallback    = StructureFactory::ProgressCallback;
    using BlockUsageCountType = StructureFactory::BlockUsageCountType;

    static constexpr size_t CALLBACK_GAP = 10000;

    StructureFactoryPrivate(
        const BlockDataMap& blockDataMap,
        TargetSurface       targetSurface,
        const Version&      blockFormatVersion)
        : blockDataMap_(blockDataMap)
        , targetSurface_(targetSurface)
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

    void setBlockFormatVersion(const Version& blockFormatVersion)
    { blockFormatVersion_ = blockFormatVersion; }

    void setFallbackBlock(const BlockDataPair* fallbackBlock = nullptr)
    { fallbackBlock_ = fallbackBlock; }

    void setProgressCallback(ProgressCallback callback = nullptr)
    { callback_ = callback; }

    void setUserdata(void* userdata = nullptr)
    { userdata_ = userdata; }

    const BlockDataMap& getBlockDataMap() const
    { return blockDataMap_; }

    BlockDataMap& getBlockDataMapRef()
    { return blockDataMap_; }

    nbt::Tag generateSingleStructure(cv::Mat image);

    nbt::Tag generateSingleStructure(ImageFramesOStream& stream);

    std::vector<nbt::Tag> generateDetachStructure(ImageFramesOStream& stream, int numThreads);

    const BlockUsageCountType& getBlockUsageCount() const
    { return blockUsageCount_; }

    void releaseCaches()
    { blockUsageCount_.clear(); }

private:
    static nbt::Tag generateSingleStructureHelper(
        ImageFramesOStream&  stream,
        const BlockDataMap&  blockDataMap,
        const TargetSurface& targetSurface,
        const Version&       blockFormatVersion,
        const BlockDataPair* fallbackBlock,
        ProgressCallback     callback,
        void*                userdata,
        BlockUsageCountType& blockUsageCount,
        const ColorKdTree&   colorKdTree
    );

    BlockDataMap         blockDataMap_;
    TargetSurface        targetSurface_;
    Version              blockFormatVersion_;
    const BlockDataPair* fallbackBlock_      = nullptr;
    ProgressCallback     callback_           = nullptr;
    void*                userdata_           = nullptr;

    BlockUsageCountType  blockUsageCount_;

    // 通过 KD 树加速最近邻颜色查找
    ColorKdTree          colorKdTree_;
};

nbt::Tag StructureFactoryPrivate::generateSingleStructure(cv::Mat image)
{
    releaseCaches();
    SingleImageFramesOStream stream(image);
    return generateSingleStructureHelper(
        stream,
        blockDataMap_,
        targetSurface_,
        blockFormatVersion_,
        fallbackBlock_,
        callback_,
        userdata_,
        blockUsageCount_,
        colorKdTree_
    );
}

nbt::Tag StructureFactoryPrivate::generateSingleStructure(ImageFramesOStream& stream)
{
    releaseCaches();
    return generateSingleStructureHelper(
        stream,
        blockDataMap_,
        targetSurface_,
        blockFormatVersion_,
        fallbackBlock_,
        callback_,
        userdata_,
        blockUsageCount_,
        colorKdTree_
    );
}

std::vector<nbt::Tag> StructureFactoryPrivate::generateDetachStructure(ImageFramesOStream& stream, int numThreads)
{
    releaseCaches();
    if (!stream.isOpened())
        return std::vector<nbt::Tag>();

    ThreadPool threadPool(numThreads);

    using TaskResult = std::pair<nbt::Tag, BlockUsageCountType>;
    std::vector<std::future<TaskResult>> results;

    while (!stream.isEnd())
    {
        cv::Mat frame = stream.nextFrame();
        if (frame.empty())
            continue;

        results.emplace_back(threadPool.submit(
            [this, frame = std::move(frame)]() mutable -> TaskResult
            {
                SingleImageFramesOStream frameStream(std::move(frame));
                BlockUsageCountType localUsageCount;
                nbt::Tag tag = generateSingleStructureHelper(
                    frameStream,
                    blockDataMap_,
                    targetSurface_,
                    blockFormatVersion_,
                    fallbackBlock_,
                    nullptr,
                    nullptr,
                    localUsageCount,
                    colorKdTree_
                );
                return TaskResult(std::move(tag), std::move(localUsageCount));
            }
        ));
    }

    std::vector<nbt::Tag> ret;
    ret.reserve(results.size());
    for (auto& fut : results)
    {
        auto [tag, localUsageCount] = fut.get();
        for (auto& [id, count] : localUsageCount)
            blockUsageCount_[id] += count;
        ret.push_back(std::move(tag));
    }

    return ret;
}

nbt::Tag StructureFactoryPrivate::generateSingleStructureHelper(
    ImageFramesOStream&  stream,
    const BlockDataMap&  blockDataMap,
    const TargetSurface& targetSurface,
    const Version&       blockFormatVersion,
    const BlockDataPair* fallbackBlock,
    ProgressCallback     callback,
    void*                userdata,
    BlockUsageCountType& blockUsageCount,
    const ColorKdTree&   colorKdTree)
{
    if (!stream.isOpened() || stream.isEnd() || stream.frameSize().empty() ||
        stream.frameCount() <= 0 || stream.frameCount() > INT_MAX)
        return nbt::Tag();

    // 创建 MC Structure NBT Tag
    nbt::be::MCStructure structure(1, 1, 1, 1);
    // 设置结构大小
    const int frameCount = static_cast<int>(stream.frameCount());
    const int frameCols  = stream.frameSize().width;
    const int frameRows  = stream.frameSize().height;
    switch (targetSurface)
    {
        case TargetSurface::Up:
        case TargetSurface::Down:
            structure.size()[0] = frameCols;
            structure.size()[1] = frameCount;
            structure.size()[2] = frameRows;
            break;
        case TargetSurface::Side:
            structure.size()[0] = frameCols;
            structure.size()[1] = frameRows;
            structure.size()[2] = frameCount;
            break;
        default:
            return nbt::Tag();
    }

    // 初始化结构方块调色板索引列表
    const int xs = structure.size()[0].getInt();
    const int ys = structure.size()[1].getInt();
    const int zs = structure.size()[2].getInt();
    const std::size_t n = xs * ys * zs;
    structure.blockIndices1().getList().resize(n, nbt::Tag(-1));
    structure.blockIndices2().getList().resize(n, nbt::Tag(-1));

    // 回调函数参数
    std::size_t current = 0;
    const std::size_t total = n;

    // 缓存调色板及其索引
    std::vector<const BlockData*> paletteCaches;
    std::unordered_map<std::string, std::size_t> paletteIdxCaches;

    const int blockFormatVersionHash = static_cast<int>(blockFormatVersion.hash());

    // 填充方块调色板索引
    auto& indices = structure.blockIndices1();
    for (int frameIdx = 0; frameIdx < frameCount; ++frameIdx)
    {
        cv::Mat frame = stream.nextFrame();
        frame = convertImageColorToBgra(frame);
        if (frame.empty() || frame.cols != frameCols || frame.rows != frameRows)
            return nbt::Tag();

        for (int row = 0; row < frameRows; ++row)
        {
            for (int col = 0; col < frameCols; ++col)
            {
                // 遍历像素点颜色，并获取颜色与之最接近的方块数据
                // 如果是透明像素，根据 fallbackBlock 值决定是否使用结构空位
                const auto rgba = frame.at<cv::Vec4b>(row, col);
                std::string id;
                const BlockData* data = nullptr;
                // Alpha 通道值低于 128 的像素视为透明像素
                if (rgba[3] < 128)
                {
                    if (fallbackBlock)
                    {
                        id   = fallbackBlock->first;
                        data = fallbackBlock->second;
                    }
                }
                else
                {
                    const Rgb rgb{rgba[2], rgba[1], rgba[0]};
                    const auto& nearest = colorKdTree.findNearest(rgb);
                    id   = nearest.first;
                    data = nearest.second;
                }

                // 默认为结构空位
                int paletteIdx = -1;
                if (data != nullptr)
                {
                    if (paletteIdxCaches.find(id) == paletteIdxCaches.end())
                    {
                        paletteCaches.push_back(data);
                        paletteIdx = paletteCaches.size() - 1;
                        paletteIdxCaches[id] = paletteIdx;
                    }
                    else
                    {
                        paletteIdx = paletteIdxCaches[id];
                    }
                }

                // 转换坐标系
                int x, y, z;
                switch (targetSurface)
                {
                    case TargetSurface::Up:
                        x = frameCols - col - 1;
                        y = frameCount - frameIdx - 1;
                        z = frameRows - row - 1;
                        break;
                    case TargetSurface::Down:
                        x = frameCols - col - 1;
                        y = frameIdx;
                        z = row;
                        break;
                    case TargetSurface::Side:
                        x = frameCols - col - 1;
                        y = frameRows - row - 1;
                        z = frameIdx;
                        break;
                    default: return nbt::Tag();
                }

                // 计算索引值
                const std::size_t indicesIdx = x * zs * ys + y * zs + z;
                indices[indicesIdx] = nbt::Tag(paletteIdx);

                // 更新方块用量信息
                ++(blockUsageCount[id]);

                // 回调函数
                ++current;
                if (callback && (current % CALLBACK_GAP == 0))
                {
                    bool stop = false;
                    callback(current, total, stop, userdata);
                    if (stop) return structure.root;
                }
            }
        }
    }

    // 填充调色板
    auto& palette = structure.blockPalette();
    for (const auto& blockData : paletteCaches)
    {
        nbt::Tag paletteItem = nbt::Tag::compound();
        paletteItem["name"]    = blockData->id;
        paletteItem["states"]  = nbt::Tag::compound();
        paletteItem["version"] = blockFormatVersionHash;
        palette.pushBack(std::move(paletteItem));
    }

    return structure.root;
}

StructureFactory::StructureFactory()
    : StructureFactory(BlockDataMap())
{}

StructureFactory::StructureFactory(
    const BlockDataMap& blockDataMap,
    TargetSurface       targetSurface,
    const Version&      blockFormatVersion)
    : ptr_(new StructureFactoryPrivate(blockDataMap, targetSurface, blockFormatVersion))
{}

StructureFactory::~StructureFactory() = default;

void StructureFactory::setBlockDataMap(const BlockDataMap& blockDataMap)
{ ptr_->setBlockDataMap(blockDataMap); }

void StructureFactory::setTargetSurface(TargetSurface targetSurface)
{ ptr_->setTargetSurface(targetSurface); }

void StructureFactory::setBlockFormatVersion(const Version& blockFormatVersion)
{ ptr_->setBlockFormatVersion(blockFormatVersion); }

void StructureFactory::setFallbackBlock(const BlockDataPair* fallbackBlock)
{ ptr_->setFallbackBlock(fallbackBlock); }

void StructureFactory::setProgressCallback(ProgressCallback callback)
{ ptr_->setProgressCallback(callback); }

void StructureFactory::setUserdata(void* userdata)
{ ptr_->setUserdata(userdata); }

const BlockDataMap& StructureFactory::getBlockDataMap() const
{ return ptr_->getBlockDataMap(); }

BlockDataMap& StructureFactory::getBlockDataMapRef()
{ return ptr_->getBlockDataMapRef(); }

nbt::Tag StructureFactory::generateSingleStructure(cv::Mat image)
{ return ptr_->generateSingleStructure(image); }

nbt::Tag StructureFactory::generateSingleStructure(ImageFramesOStream& stream)
{ return ptr_->generateSingleStructure(stream); }

std::vector<nbt::Tag> StructureFactory::generateDetachStructure(ImageFramesOStream& stream, int numThreads)
{ return ptr_->generateDetachStructure(stream, numThreads); }

const StructureFactory::BlockUsageCountType& StructureFactory::getBlockUsageCount() const
{ return ptr_->getBlockUsageCount(); }

void StructureFactory::releaseCaches()
{ ptr_->releaseCaches(); }
