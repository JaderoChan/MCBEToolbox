#pragma once

#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map

#include <opencv2/core/mat.hpp> // cv::Mat

#include "base_factory.hpp"
#include "image_frames_ostream.hpp"
#include "video_frames_ostream.hpp"

class BlockImageFactory : public BaseFactory
{
public:
    using BlockTextureMap = std::unordered_map<std::string, cv::Mat>;

    static constexpr const char* DEFAULT_TEXTURES_DIR_PATH = "./textures";
    // 生成方块视频时，输入视频帧的最大尺寸。
    static constexpr int VIDEO_FRAME_MAX_WIDTH  = 2048 / 16;
    static constexpr int VIDEO_FRAME_MAX_HEIGHT = 2048 / 16;

    BlockImageFactory(
        const BlockDataMap& blocks,
        SurfaceDirection    desiredSurface,
        std::string_view    texturesDirPath = DEFAULT_TEXTURES_DIR_PATH);

    void setTexturesDirPath(std::string_view texturesDirPath) { texturesDirPath_ = texturesDirPath; }
    std::string getTexturesDirPath() const                    { return texturesDirPath_;            }

    cv::Mat generateBlockImage(ImageFramesOStream& stream);
    bool generateBlockVideo(VideoFramesOStream& stream, const std::string& outFilePath, int numThreads = 4);

private:
    std::string createTexturePath(const std::string& path) { return texturesDirPath_ + "/" + path; }

    cv::Mat generateBlockImageHelper(
        FramesOStream&   stream,
        ProgressCallback callback,
        void*            userdata,
        BlockUsageMap&   blockUsageMap,
        BlockTextureMap& texturesCache,
        bool             callbackPerFrame);

    std::string     texturesDirPath_;
    BlockTextureMap texturesCache_;
};
