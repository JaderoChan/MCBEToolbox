#pragma once

#include <vector> // std::vector

#include <mcnbt/mcnbt.hpp> // nbt::Tag

#include "base_factory.hpp"
#include "image_frames_ostream.hpp"
#include "version.hpp"
#include "video_frames_ostream.hpp"

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

    nbt::Tag generateSingleMCStructure(ImageFramesOStream& stream);
    nbt::Tag generateSingleMCStructure(VideoFramesOStream& stream);
    std::vector<nbt::Tag> generateDetachMCStructure(VideoFramesOStream& stream, int numThreads = 4);

private:
    nbt::Tag generateSingleMCStructureHelper(
        FramesOStream&   stream,
        ProgressCallback callback,
        void*            userdata,
        BlockUsageMap&   blockUsageCount,
        bool             useFrameIndexCallback);

    Version blockFormatVersion_;
};
