#pragma once

#include <atomic>  // std::atomic
#include <string>  // std::string
#include <utility> // std::pair
#include <vector>  // std::vector

#include <opencv2/core/mat.hpp> // cv::Mat
#include <mcnbt/mcnbt.hpp>      // nbt::Tag

#include "base_factory.hpp"
#include "single_image_ostream.hpp"
#include "version.hpp"
#include "video_image_ostream.hpp"

class MCStructureFactory : public BaseFactory
{
public:
    static constexpr Version DEFAULT_BLOCK_FORMAT_VERSION = Version(1, 21, 0, 7);

    MCStructureFactory(
        const BlockDataMap& blocks,
        SurfaceDirection    desiredSurface,
        const Version&      blockFormatVersion = DEFAULT_BLOCK_FORMAT_VERSION);

    void setBlockFormatVersion(const Version& blockFormatVersion) { blockFormatVersion_ = blockFormatVersion; }
    Version getBlockFormatVersion() const                         { return blockFormatVersion_;               }

    nbt::Tag generateSingleMCStructure(ImageOStream& stream, bool callbackPerFrame);
    std::vector<nbt::Tag> generateDetachMCStructure(ImageOStream& stream, int numThreads = 4);
    bool generateDetachMCStructure(ImageOStream& stream, const std::string& outDirPath, int numThreads = 4);

private:
    nbt::Tag generateSingleMCStructureHelper(
        ImageOStream&    stream,
        ProgressCallback callback,
        void*            userdata,
        BlockUsageMap&   blockUsageMap,
        bool             callbackPerFrame);

    std::pair<nbt::Tag, BlockUsageMap> processDetachFrame(const cv::Mat& frame, std::atomic<bool>& shouldStop);

    Version blockFormatVersion_;
};
