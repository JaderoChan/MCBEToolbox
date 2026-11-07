#pragma once

#include <atomic>     // std::atomic
#include <filesystem> // std::filesystem::path
#include <utility>    // std::pair
#include <vector>     // std::vector

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

    ResultType generateSingleMCStructure(
        ImageOStream& stream, bool callbackPerFrame, nbt::Tag& outStructure);
    ResultType generateDetachMCStructure(
        ImageOStream& stream, int numThreads, std::vector<nbt::Tag>& outStructures);
    ResultType generateDetachMCStructure(
        ImageOStream& stream, const std::filesystem::path& outDirPath,
        const std::filesystem::path& fileName, int numThreads = 4);

private:
    ResultType generateSingleMCStructureHelper(
        ImageOStream&    stream,
        ProgressCallback callback,
        void*            userdata,
        BlockUsageMap&   blockUsageMap,
        bool             callbackPerFrame,
        nbt::Tag&        outStructure);

    std::pair<nbt::Tag, BlockUsageMap> processDetachFrame(const cv::Mat& frame, std::atomic<bool>& shouldStop);

    Version blockFormatVersion_;
};
