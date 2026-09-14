#pragma once

#include <string> // std::string
#include <vector> // std::vector

#include "base_factory.hpp"
#include "image_frames_ostream.hpp"
#include "video_frames_ostream.hpp"

class MCFunctionFactory : public BaseFactory
{
public:
    using MCFunction = std::vector<std::string>;

    MCFunctionFactory(const BlockDataMap& blocks, SurfaceDirection desiredSurface);

    MCFunction generateSingleMCFunction(ImageFramesOStream& stream);
    MCFunction generateSingleMCFunction(VideoFramesOStream& stream);
    std::vector<MCFunction> generateDetachMCFunction(VideoFramesOStream& stream, int numThreads = 4);

private:
    MCFunction generateSingleMCFunctionHelper(
        FramesOStream&   stream,
        ProgressCallback callback,
        void*            userdata,
        BlockUsageMap&   blockUsageCount,
        bool             useFrameIndexCallback);
};
