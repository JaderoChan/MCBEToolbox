#include "assist_function_generator.hpp"

#include <algorithm>

#include <format_string.hpp>

namespace
{

#define MCBE_TOOLBOX_NAMESPACE       "mcbetoolbox"
#define ORIGIN_NAME                  MCBE_TOOLBOX_NAMESPACE "_origin"
#define ORIGIN_ARMOR_STAND_ID       "@e[type=minecraft:armor_stand,name=" ORIGIN_NAME "]"
#define WATCH_ANCHOR_NAME            MCBE_TOOLBOX_NAMESPACE "_anchor"
#define WATCH_ANCHOR_ARMOR_STAND_ID "@e[type=minecraft:armor_stand,name=" ORIGIN_NAME "]"

constexpr double WATCH_DISTANCE_AND_CANVAS_SIZE_RATE = 0.75;

} // namespace

AssistFunctionGenerator::AssistFunctionGenerator(int width, int height, SurfaceDirection surfaceDirection, std::string structureName)
    : width_(width), height_(height), surfaceDirection_(surfaceDirection), structureName_(structureName)
{
    // 计算一个常加载区域的“宽度”（以区块 16*16 为单位）。
    const int areaWidthChunks  = width / 16 + 1;
    if (areaWidthChunks > 100)
    {
        // 暂时假定“宽度”不会超过 100 个区块长度。
        throw std::runtime_error("AssistFunctionGenerator::AssistFunctionGenerator() The ticking area over 100 chunks size");
    }

    if (surfaceDirection_ == SURFACE_DIRECTION_UP || surfaceDirection_ == SURFACE_DIRECTION_BOTTOM)
    {
        // 计算一个常加载区域的“高度”（一个常加载区域最多包含 100 个区块）。
        const int areaHeightChunks = 100 / areaWidthChunks;
        const int areaHeightBlocks = areaHeightChunks * 16;
        // 计算需要多少个常加载区域能够覆盖整个结构。
        const int areaCount        = height / areaHeightBlocks + 1;

        // 计算各个常加载区域的具体坐标范围。
        for (int i = 0, lastPosZ = 0; i < areaCount; ++i, lastPosZ += (areaHeightBlocks + 1))
        {
            TickArea area;
            area.pos1 = {0,     0, lastPosZ                   };
            area.pos2 = {width, 0, lastPosZ + areaHeightBlocks};
            area.name = formatString("{}_ticking_area_{}", MCBE_TOOLBOX_NAMESPACE, i);
            tickingAreaList_.emplace_back(std::move(area));
        }
    }
    else
    {
        TickArea area;
        area.pos1 = {0, 0, 0};
        area.pos2 =
            (surfaceDirection_ == SURFACE_DIRECTION_NORTH || surfaceDirection_ == SURFACE_DIRECTION_SOUTH)
            ? std::array<int, 3>{width, 0, 0    }
            : std::array<int, 3>{0,     0, width};
        area.name = formatString("{}_ticking_area_{}", MCBE_TOOLBOX_NAMESPACE, 0);
        tickingAreaList_.emplace_back(std::move(area));
    }
}

AssistFunctionGenerator::MCFunction AssistFunctionGenerator::makeSetOriginFunction()
{
    MCFunction function;

    // 清空先前的常加载区域。
    for (const auto& area : tickingAreaList_)
        function.emplace_back(formatString("tickingarea remove {}", area.name));
    // 删除之前的原点。
    function.emplace_back(formatString("execute as {} at @s if block ~~-1~ minecraft:glass run setblock ~~-1~ minecraft:air", ORIGIN_ARMOR_STAND_ID));
    function.emplace_back(formatString("kill {}", ORIGIN_ARMOR_STAND_ID));
    // 添加新的原点。
    function.emplace_back("setblock ~~~ minecraft:glass");
    function.emplace_back(formatString("summon minecraft:armor_stand {} ~~1~", ORIGIN_NAME));
    // 添加新常加载区域。
    for (const auto& area : tickingAreaList_)
        function.emplace_back(formatString("execute as {} at @s run tickingarea add {} {} {}", ORIGIN_ARMOR_STAND_ID, posString(area.pos1), posString(area.pos2), area.name));

    return function;
}

AssistFunctionGenerator::MCFunction AssistFunctionGenerator::makeClearOriginFunction()
{
    MCFunction function;

    for (const auto& area : tickingAreaList_)
        function.emplace_back(formatString("tickingarea remove {}", area.name));
    function.emplace_back(formatString("execute as {} at @s if block ~~-1~ minecraft:glass run setblock ~~-1~ minecraft:air", ORIGIN_ARMOR_STAND_ID));
    function.emplace_back(formatString("kill {}", ORIGIN_ARMOR_STAND_ID));

    return function;
}

AssistFunctionGenerator::MCFunction AssistFunctionGenerator::makeBuildFunction()
{
    return MCFunction
    {
        formatString("execute as {} at @s run structure load {} ~~~", ORIGIN_ARMOR_STAND_ID, structureName_)
    };
}

AssistFunctionGenerator::MCFunction AssistFunctionGenerator::makeMoveToWatchPointFunction()
{
    const int halfWidth     = width_  * 0.5;
    const int halfHeight    = height_ * 0.5;
    const int watchDistance = std::max(width_, height_) * WATCH_DISTANCE_AND_CANVAS_SIZE_RATE;

    std::array<int, 3> anchorOffset = {0, 0, 0};
    std::array<int, 3> playerOffset = {0, 0, 0};
    switch (surfaceDirection_)
    {
        case SURFACE_DIRECTION_UP:
            anchorOffset = {halfWidth, 0,              halfHeight};
            playerOffset = {halfWidth, watchDistance,  halfHeight};
            break;
        case SURFACE_DIRECTION_BOTTOM:
            anchorOffset = {halfWidth, 0,              halfHeight};
            playerOffset = {halfWidth, -watchDistance, halfHeight};
            break;
        case SURFACE_DIRECTION_NORTH:
            anchorOffset = {halfWidth, halfHeight, 0             };
            playerOffset = {halfWidth, halfHeight, -watchDistance};
            break;
        case SURFACE_DIRECTION_SOUTH:
            anchorOffset = {halfWidth, halfHeight, 0            };
            playerOffset = {halfWidth, halfHeight, watchDistance};
            break;
        case SURFACE_DIRECTION_EAST:
            anchorOffset = {0,              halfHeight, halfWidth};
            playerOffset = {watchDistance,  halfHeight, halfWidth};
            break;
        case SURFACE_DIRECTION_WEST:
            anchorOffset = {0,              halfHeight, halfWidth};
            playerOffset = {-watchDistance, halfHeight, halfWidth};
            break;
        default:
            break;
    }

    return MCFunction
    {
        formatString("execute as {} at @s run summon minecraft:armor_stand {} {}", ORIGIN_ARMOR_STAND_ID, WATCH_ANCHOR_NAME, posString(anchorOffset)),
        formatString("execute as @s at {} run tp @s {} facing {}", ORIGIN_ARMOR_STAND_ID, posString(playerOffset), WATCH_ANCHOR_ARMOR_STAND_ID),
        formatString("kill {}", WATCH_ANCHOR_ARMOR_STAND_ID)
    };
}

std::string AssistFunctionGenerator::posString(const std::array<int, 3>& pos)
{
    return formatString("~{}~{}~{}", pos[0], pos[1], pos[2]);
}
