#pragma once

#include <array>
#include <string>
#include <vector>

#include <block.hpp>

/** 当前仅适用于图像生成的结构或以 Detached File 模式产出的视频结构。 */
class AssistFunctionGenerator
{
public:
    using MCFunction = std::vector<std::string>;

    AssistFunctionGenerator(int width, int height, SurfaceDirection surfaceDirection, std::string structureName);

    MCFunction makeSetOriginFunction();
    MCFunction makeClearOriginFunction();
    MCFunction makeBuildFunction();
    MCFunction makeMoveToWatchPointFunction();

private:
    int              width_;
    int              height_;
    SurfaceDirection surfaceDirection_;
    std::string      structureName_;

    struct TickArea
    {
        TickArea() = default;

        std::array<int, 3> pos1 = {0, 0, 0};
        std::array<int, 3> pos2 = {0, 0, 0};
        std::string        name;
    };

    static std::string posString(const std::array<int, 3>& pos);

    std::vector<TickArea> tickingAreaList_;
};
