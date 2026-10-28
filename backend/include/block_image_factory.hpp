#pragma once

#include <string>        // std::string
#include <unordered_map> // std::unordered_map

#include <opencv2/core/mat.hpp> // cv::Mat

#include "base_factory.hpp"
#include "single_image_ostream.hpp"
#include "video_image_ostream.hpp"

class BlockImageFactory : public BaseFactory
{
public:
    using BlockTextureMap = std::unordered_map<std::string, cv::Mat>;

    static constexpr const char* DEFAULT_TEXTURES_DIR_PATH = "./textures";
    // 生成方块视频时，输入视频帧的最大尺寸。
    static constexpr int VIDEO_FRAME_MAX_WIDTH  = 2048 / 16;
    static constexpr int VIDEO_FRAME_MAX_HEIGHT = 2048 / 16;

    BlockImageFactory(
        const BlockDataMap&          blocks,
        SurfaceDirection             desiredSurface,
        const std::filesystem::path& texturesDirPath = DEFAULT_TEXTURES_DIR_PATH);

    void setTexturesDirPath(const std::filesystem::path& texturesDirPath) { texturesDirPath_ = texturesDirPath; }
    std::filesystem::path getTexturesDirPath() const                      { return texturesDirPath_;            }

    cv::Mat generateBlockImage(SingleImageOStream& stream);
    bool generateBlockVideo(VideoImageOStream& stream, const std::filesystem::path& outFilePath, int numThreads = 4);

private:
    std::filesystem::path createTexturePath(const std::string& path);

    cv::Mat generateBlockImageHelper(
        ImageOStream&    stream,
        ProgressCallback callback,
        void*            userdata,
        BlockUsageMap&   blockUsageMap,
        BlockTextureMap& texturesCache,
        bool             callbackPerFrame);

    std::filesystem::path texturesDirPath_;
    BlockTextureMap       texturesCache_;
};
