#include "mcbe_toolbox_api.hpp"

#include <assert.h>
#include <stdexcept>
#include <unordered_map>
#include <vector>

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
    using Tree = nanoflann::KDTreeSingleIndexAdaptor<
        nanoflann::L2_Simple_Adaptor<float, RgbCloud>,
        RgbCloud,
        3>;

    RgbCloud cloud;
    Tree     tree;
    std::vector<std::pair<std::string_view, const BlockData*>> blockDataVec;

    explicit ColorKdTree(const BlockDataMap& blockDataMap, TargetSurface targetSurface)
        : tree(3, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(
            10, nanoflann::KDTreeSingleIndexAdaptorFlags::SkipInitialBuildIndex))
    {
        cloud.pts.reserve(blockDataMap.size());
        blockDataVec.reserve(blockDataMap.size());
        for (const auto& [id, data] : blockDataMap)
        {
            cloud.pts.push_back(getSurface(data->surface, targetSurface).second);
            blockDataVec.emplace_back(id, data);
        }
        tree.buildIndex();
    }

    std::pair<std::string_view, const BlockData*>
    findNearest(const Rgb& query) const
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

inline std::string concatTexturePath(
    const std::string& texturePath,
    const std::string& basePath = "./textures")
{
    return basePath + "/" + texturePath;
}

} // namespace

cv::Mat convertImageToBlockImage(
    const cv::Mat&                           image,
    const BlockDataMap&                      blockDataMap,
    TargetSurface                            targetSurface,
    const std::pair<std::string, BlockData>* fallbackBlock,
    std::map<std::string, std::size_t>*      blockUsageCount,
    ProgressCallback                         callback,
    void*                                    userdata)
{
    if (image.type() != CV_8UC1 && image.type() != CV_8UC3 && image.type() != CV_8UC4)
        return cv::Mat();
    cv::Mat img;
    convertColorToBgra(image, img);
    if (img.empty() || blockDataMap.empty())
        return cv::Mat();

    // 构建 KD 树用于加速最近邻颜色查找
    ColorKdTree colorKdTree(blockDataMap, targetSurface);
    // 缓存材质文件路径与材质
    std::unordered_map<std::string, cv::Mat> cache;

    // 用于回调函数
    // 每处理 10000 个像素执行一次回调函数
    constexpr std::size_t GAP = 10000;
    std::size_t current = 0;
    const std::size_t total = img.rows * img.cols;

    // 假定所有材质图片尺寸为 16*16，所以每个像素对应 16*16 的方块材质区域
    cv::Mat ret(img.rows * 16, img.cols * 16, CV_8UC4, cv::Scalar(0.0, 0.0, 0.0, 0.0));
    for (int row = 0; row < img.rows; ++row)
    {
        for (int col = 0; col < img.cols; ++col)
        {
            // 遍历像素点颜色，并获取颜色与之最接近的方块数据
            // 如果是透明像素，根据 fallbackBlock 值决定是否跳过填充
            const auto rgba = img.at<cv::Vec4b>(row, col);
            std::string id;
            const BlockData* data = nullptr;
            // Alpha 通道值低于 128 的像素视为透明像素
            if (rgba[3] < 128)
            {
                if (fallbackBlock)
                {
                    id   = fallbackBlock->first;
                    data = &fallbackBlock->second;
                }
            }
            else
            {
                const Rgb rgb{rgba[2], rgba[1], rgba[0]};
                const auto& nearest = colorKdTree.findNearest(rgb);
                id   = nearest.first;
                data = nearest.second;
            }

            if (data != nullptr)
            {
                // 如果当前材质还未被加载则将其加载至缓存中
                const std::string texturePath = concatTexturePath(
                    getSurface(data->surface, targetSurface).first);
                if (cache.find(texturePath) == cache.end())
                {
                    cv::Mat texture = cv::imread(texturePath, cv::IMREAD_UNCHANGED);
                    if (!texture.empty())
                        convertColorToBgra(texture, texture);
                    if (texture.empty())
                        texture = cv::Mat(16, 16, CV_8UC4, cv::Scalar(0.0, 0.0, 0.0, 255.0));
                    cache[texturePath] = texture;
                }

                // 直接从缓存中加载方块材质
                cv::Mat texture = cache[texturePath];
                // 复制方块材质至像素映射区域
                texture.copyTo(ret(
                    cv::Range(row * 16, row * 16 + 16),
                    cv::Range(col * 16, col * 16 + 16)));

                // 更新方块用量信息
                if (blockUsageCount) ++(*blockUsageCount)[id];
            }

            // 回调函数
            ++current;
            if (callback && (current % GAP == 0))
            {
                bool stop = false;
                callback(current, total, stop, userdata);
                if (stop) return ret;
            }
        }
    }

    return ret;
}
