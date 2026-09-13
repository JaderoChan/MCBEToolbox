#include <block_image_factory.hpp>

#include <assert.h> // assert

#include <opencv2/imgcodecs.hpp> // cv::imread

#include <image_utilities.hpp>

BlockImageFactory::BlockImageFactory(
    const BlockDataMap& blocks,
    SurfaceDirection    desiredSurface,
    std::string_view    texturesDirPath)
    : BaseFactory(blocks, desiredSurface), texturesDirPath_(texturesDirPath)
{}

cv::Mat BlockImageFactory::generateBlockImage(ImageFramesOStream& stream)
{
    reset();

    if (!stream.isOpened() || stream.isEnd() || !isConfigured())
        return cv::Mat();
    cv::Mat image = stream.nextFrame();
    if (image.empty())
        return cv::Mat();

    assert(image.type() == CV_8UC4);

    const std::size_t total = image.rows * image.cols;

    // 假定所有材质图片尺寸为 16*16，所以每个像素对应 16*16 的方块材质区域
    cv::Mat ret(image.rows * 16, image.cols * 16, CV_8UC4, cv::Scalar(0.0, 0.0, 0.0, 0.0));
    for (int row = 0; row < image.rows; ++row)
    {
        for (int col = 0; col < image.cols; ++col)
        {
            const auto color = image.at<cv::Vec4b>(row, col);
            auto [id, block] = retrieveAppropriateBlock(color);

            if (block)
            {
                const auto& surface = block->surfaceData.get(desiredSurface_);
                if (!surface.first.empty())
                {
                    const std::string texturePath = createTexturePath(surface.first);

                    // 如果当前材质还未被加载则将其加载至缓存中
                    if (texturesCache_.find(texturePath) == texturesCache_.end())
                    {
                        cv::Mat texture = cv::imread(texturePath, cv::IMREAD_UNCHANGED);
                        if (!texture.empty())
                            texture = convertColorToBgra(texture);
                        if (texture.empty())
                            texture = cv::Mat(16, 16, CV_8UC4, cv::Scalar(0.0, 0.0, 0.0, 0.0));
                        texturesCache_[texturePath] = texture;
                    }

                    // 直接从缓存中加载方块材质
                    cv::Mat texture = texturesCache_[texturePath];
                    // 复制方块材质至像素映射区域
                    texture.copyTo(ret(
                        cv::Range(row * 16, row * 16 + 16),
                        cv::Range(col * 16, col * 16 + 16)));
                }

                // 更新方块用量信息
                updateBlockUsageCount(id, 1);
            }

            // 回调函数
            if (executeCallback(row * image.cols + col + 1, total))
                return cv::Mat();
        }
    }

    return ret;
}
