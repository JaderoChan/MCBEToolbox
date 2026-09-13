#pragma once

#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map

#include <opencv2/core/mat.hpp> // cv::Mat

#include "image_frames_ostream.hpp"
#include "base_factory.hpp"

class BlockImageFactory : public BaseFactory
{
public:
    using BlockTextureMap = std::unordered_map<std::string, cv::Mat>;

    static constexpr const char* DEFAULT_TEXTURES_DIR_PATH = "./textures";

    BlockImageFactory(
        const BlockDataMap& blocks,
        SurfaceDirection    desiredSurface,
        std::string_view    texturesDirPath = DEFAULT_TEXTURES_DIR_PATH);

    void setTexturesDirPath(std::string_view texturesDirPath) { texturesDirPath_ = texturesDirPath; }
    std::string getTexturesDirPath() const                    { return texturesDirPath_;            }

    cv::Mat generateBlockImage(ImageFramesOStream& stream);

private:
    std::string createTexturePath(const std::string& path) { return texturesDirPath_ + "/" + path; }

    std::string     texturesDirPath_;
    BlockTextureMap texturesCache_;
};
