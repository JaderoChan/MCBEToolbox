#pragma once

#include <atomic>  // std::atomic
#include <string>  // std::string
#include <utility> // std::pair
#include <vector>  // std::vector

#include <opencv2/core/mat.hpp> // cv::Mat

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
    std::vector<MCFunction> generateDetachMCFunction(FramesOStream& stream, int numThreads = 4);
    bool generateDetachMCFunction(FramesOStream& stream, const std::string& outDirPath, int numThreads = 4);

private:
    MCFunction generateSingleMCFunctionHelper(
        FramesOStream&   stream,
        ProgressCallback callback,
        void*            userdata,
        BlockUsageMap&   blockUsageCount,
        bool             useFrameIndexCallback);

    std::pair<MCFunction, BlockUsageMap> processDetachFrame(const cv::Mat& frame, std::atomic<bool>& shouldStop);
};
