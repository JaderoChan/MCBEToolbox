#pragma once

#include <atomic>  // std::atomic
#include <string>  // std::string
#include <utility> // std::pair
#include <vector>  // std::vector

#include <opencv2/core/mat.hpp> // cv::Mat

#include "base_factory.hpp"
#include "single_image_ostream.hpp"
#include "video_image_ostream.hpp"

class MCFunctionFactory : public BaseFactory
{
public:
    // 每个 .mcfunction 文件最多包含 10000 条指令，此处预留 1000 条以支持一定程度的链式调用。
    static constexpr std::size_t MAX_COMMAND_COUNT_PER_MCFUNCTION = 9000;

    using MCFunction = std::vector<std::string>;

    MCFunctionFactory(const BlockDataMap& blocks, SurfaceDirection desiredSurface);

    MCFunction generateSingleMCFunction(ImageOStream& stream, bool callbackPerFrame);
    std::vector<MCFunction> generateDetachMCFunction(ImageOStream& stream, int numThreads = 4);
    bool generateDetachMCFunction(ImageOStream& stream, const std::string& outDirPath, int numThreads = 4);

private:
    MCFunction generateSingleMCFunctionHelper(
        ImageOStream&    stream,
        ProgressCallback callback,
        void*            userdata,
        BlockUsageMap&   blockUsageMap,
        bool             callbackPerFrame);

    std::pair<MCFunction, BlockUsageMap> processDetachFrame(const cv::Mat& frame, std::atomic<bool>& shouldStop);
};
