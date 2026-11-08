#include "assist_function_generator.hpp"

#include <algorithm>

#include <format_string.hpp>

namespace
{

// 观看距离与生成的结构尺寸的比率，用于计算应该把用户传送到多远的距离以能够观看到整个结构。
constexpr double WATCH_DISTANCE_AND_STRUCTURE_SIZE_RATE = 0.75;

} // namespace

std::string Position::toString(CoordType type) const
{
    switch (type)
    {
        case ABSOLUTE: return formatString("{} {} {}",    x, y, z);
        case RELATIVE: return formatString("~{} ~{} ~{}", x, y, z);
        case LOCAL:    return formatString("^{} ^{} ^{}", x, y, z);
        default:       return std::string();
    }
}

AssistFunctionGenerator::AssistFunctionGenerator(
    int frameWidth, int frameHeight, SurfaceDirection surfaceDirection, std::string structureName)
    : frameWidth_(frameWidth), frameHeight_(frameHeight), surfaceDirection_(surfaceDirection)
    , structureName_(std::move(escapeQuotes(structureName)))
    , normalOriginName_(  formatString("\"{}_origin\"",                          structureName_   ))
    , normalOriginNameId_(formatString("@e[type=minecraft:armor_stand,name={}]", normalOriginName_))
    , baseOriginName_(    formatString("\"{}_base_origin\"",                     structureName_   ))
    , baseOriginNameId_(  formatString("@e[type=minecraft:armor_stand,name={}]", baseOriginName_  ))
    , originTag_(         formatString("\"{}_origin\"",                          structureName_   ))
    , originTagId_(       formatString("@e[type=minecraft:armor_stand,tag={}]",  originTag_       ))
{
    // 计算一个常加载区域的边长一。
    const int frameWidthChunks = frameWidth / 16 + 1;
    const int areaWidthChunks  = frameWidthChunks > 100 ? 100 : frameWidthChunks;
    const int areaWidthBlocks  = areaWidthChunks * 16;

    if (surfaceDirection_ == SURFACE_DIRECTION_UP || surfaceDirection_ == SURFACE_DIRECTION_BOTTOM)
    {
        // 计算一个常加载区域的边长二（一个常加载区域最多包含 100 个区块）。
        const int areaHeightChunks = 100 / areaWidthChunks;
        const int areaHeightBlocks = areaHeightChunks * 16;

        const int cols = frameWidth  / areaWidthBlocks  + 1;
        const int rows = frameHeight / areaHeightBlocks + 1;

        // 计算各个常加载区域的具体坐标范围。
        for (int row = 0; row < rows; ++ row)
        {
            const int zFrom = row * areaHeightBlocks;
            const int zTo   = zFrom + areaHeightBlocks - 1;
            for (int col = 0; col < cols; ++col)
            {
                const int xFrom = col   * areaWidthBlocks;
                const int xTo   = xFrom + areaWidthBlocks - 1;

                TickArea area;
                area.from = {xFrom, 0, zFrom};
                area.to   = {xTo,   0, zTo  };
                area.name = formatString("\"{}_ticking_area_{}-{}\"", structureName_, row, col);
                tickingAreaList_.emplace_back(std::move(area));
            }
        }

    }
    else
    {
        // 因为是“竖直”的墙面，所以所有常加载区域分布就是一条一区块宽的线性区域，故不存在 rows。
        const int  cols  = frameWidth / areaWidthBlocks + 1;
        // SURFACE_DIRECTION_EAST | SURFACE_DIRECTION_WEST 产生的结构与地面的交线平行于 Z 轴，
        // 因此可以根据结构的走向计算常加载区域线性区域的走向。
        const bool lineZ = (surfaceDirection_ == SURFACE_DIRECTION_EAST || surfaceDirection_ == SURFACE_DIRECTION_WEST);
        for (int col = 0; col < cols; ++col)
        {
            const int from = col * areaWidthBlocks;
            const int to   = from + areaWidthBlocks - 1;

            TickArea area;
            area.from = lineZ ? Position{0, 0, from} : Position{from, 0, 0};
            area.to   = lineZ ? Position{0, 0, to  } : Position{to,   0, 0};
            area.name = formatString("\"{}_tickingarea_{}\"", structureName_, col);
            tickingAreaList_.emplace_back(std::move(area));
        }
    }
}

AssistFunctionGenerator:: MCFunction AssistFunctionGenerator::generateSetOriginFunction() const
{
    MCFunction function;

    // 尝试清空先前的原点。
    subjoin(function, generateClearOriginFunction());

    const std::string needBaseTag = formatString("\"{}_need_base\"", structureName_);

    // 添加新的原点。
    function.emplace_back(formatString("execute as @s at @s if block ~ ~-1 ~ minecraft:air run tag @s add {}",                needBaseTag));
    function.emplace_back(formatString("execute as @s at @s if entity @s[tag={}] run setblock ~ ~-1 ~ glass",                 needBaseTag));
    function.emplace_back(formatString("execute as @s at @s if entity @s[tag={}] run summon minecraft:armor_stand {} ~ ~ ~",  needBaseTag, baseOriginName_));
    function.emplace_back(formatString("execute as @s at @s if entity @s[tag={}] run tag {} add {}",                          needBaseTag, baseOriginNameId_, originTag_));
    function.emplace_back(formatString("execute as @s at @s if entity @s[tag={}] run tag @s remove {}",                       needBaseTag, needBaseTag));
    function.emplace_back(formatString("execute as @s at @s if entity @s[tag=!{}] run summon minecraft:armor_stand {} ~ ~ ~", needBaseTag, normalOriginName_));
    function.emplace_back(formatString("execute as @s at @s if entity @s[tag=!{}] run tag {} add {}",                         needBaseTag, normalOriginNameId_, originTag_));

    // 添加新常加载区域。
    for (const auto& area : tickingAreaList_)
        function.emplace_back(formatString("execute as {} at @s run tickingarea add {} {} {}", originTagId_, area.from.toString(), area.to.toString(), area.name));

    return function;
}

AssistFunctionGenerator::MCFunction AssistFunctionGenerator::generateClearOriginFunction() const
{
    MCFunction function;

    // 清空先前的常加载区域。
    for (const auto& area : tickingAreaList_)
        function.emplace_back(formatString("tickingarea remove {}", area.name));

    // 删除之前的原点。
    function.emplace_back(formatString("execute as {} at @s if block ~ ~-1 ~ minecraft:glass run setblock ~ ~-1 ~ minecraft:air", baseOriginNameId_));
    function.emplace_back(formatString("execute as {} run kill @s",                                                               originTagId_));

    return function;
}

AssistFunctionGenerator::MCFunction AssistFunctionGenerator::generateMoveToWatchPointFunction() const
{
    const int halfWidth     = frameWidth_  * 0.5;
    const int halfHeight    = frameHeight_ * 0.5;
    const int watchDistance = std::max(
        static_cast<int>(std::max(frameWidth_, frameHeight_) * WATCH_DISTANCE_AND_STRUCTURE_SIZE_RATE),
        1
    );

    Position anchorPoint = {0, 0, 0};
    Position watchPoint  = {0, 0, 0};
    switch (surfaceDirection_)
    {
        case SURFACE_DIRECTION_UP:
            anchorPoint = {halfWidth,  0,             halfHeight};
            watchPoint  = {halfWidth,  watchDistance, halfHeight};
            break;
        case SURFACE_DIRECTION_BOTTOM:
            anchorPoint = {halfWidth, 0,              halfHeight};
            watchPoint  = {halfWidth, -watchDistance, halfHeight};
            break;
        case SURFACE_DIRECTION_NORTH:
            anchorPoint = {halfWidth, halfHeight,  0            };
            watchPoint  = {halfWidth, halfHeight, -watchDistance};
            break;
        case SURFACE_DIRECTION_SOUTH:
            anchorPoint = {halfWidth, halfHeight,  0            };
            watchPoint  = {halfWidth, halfHeight,  watchDistance};
            break;
        case SURFACE_DIRECTION_EAST:
            anchorPoint = { 0,             halfHeight, halfWidth};
            watchPoint  = { watchDistance, halfHeight, halfWidth};
            break;
        case SURFACE_DIRECTION_WEST:
            anchorPoint = {0,              halfHeight, halfWidth};
            watchPoint  = {-watchDistance, halfHeight, halfWidth};
            break;
        default:
            break;
    }

    return MCFunction
    {
        formatString("execute as @s at {} run tp @s {} facing {}", originTagId_, watchPoint.toString(), anchorPoint.toString()),
    };
}

AssistFunctionGenerator::MCFunction AssistFunctionGenerator::generateDestroyFunction() const
{
    Position to = {0, 0, 0};
    switch (surfaceDirection_)
    {
        case SURFACE_DIRECTION_UP:     // Fallthrough
        case SURFACE_DIRECTION_BOTTOM:
            to = {frameWidth_ - 1, 0, frameHeight_ - 1};
            break;
        case SURFACE_DIRECTION_NORTH:  // Fallthrough
        case SURFACE_DIRECTION_SOUTH:
            to = {frameWidth_ - 1, frameHeight_ - 1, 0};
            break;
        case SURFACE_DIRECTION_EAST:   // Fallthrough
        case SURFACE_DIRECTION_WEST:
            to = {0, frameHeight_ - 1, frameWidth_ - 1};
            break;
        default:
            break;

    }

    return MCFunction
    {
        formatString("execute as {} at @s run fill ~ ~ ~ {} minecraft:air", originTagId_, to.toString())
    };
}

void AssistFunctionGenerator::subjoin(MCFunction& out, const MCFunction& supplement)
{
    for (const auto& command : supplement)
        out.push_back(command);
}

ImageAssistFunctionGenerator::MCFunction ImageAssistFunctionGenerator::generateBuildFunction() const
{
    return MCFunction
    {
        formatString("execute as {} at @s run structure load \"{}\" ~ ~ ~", originTagId_, structureName_)
    };
}

VideoAssistFunctionGenerator::VideoAssistFunctionGenerator(
    int frameWidth, int frameHeight, SurfaceDirection surfaceDirection,
    std::string structureName, int frameCount)
    : AssistFunctionGenerator(frameWidth, frameHeight, surfaceDirection, std::move(structureName))
    , frameCount_(frameCount)
    , scoreObjectName_(formatString("\"{}_frame_index\"",                           structureName_))
    , playingTag_(     formatString("\"{}_playing\"",                               structureName_))
    , playingTagId_(   formatString("@e[type=minecraft:armor_stand,tag={},tag={}]", originTag_, playingTag_))
{}

VideoAssistFunctionGenerator::MCFunction VideoAssistFunctionGenerator::generateSetOriginFunction() const
{
    MCFunction function;

    subjoin(function, AssistFunctionGenerator::generateSetOriginFunction());
    function.emplace_back(formatString("scoreboard objectives add {} dummy", scoreObjectName_));
    function.emplace_back(formatString("scoreboard players set {} {} 0",     originTagId_, scoreObjectName_));

    return function;
}

VideoAssistFunctionGenerator::MCFunction VideoAssistFunctionGenerator::generateClearOriginFunction() const
{
    MCFunction function;

    function.emplace_back(formatString("scoreboard players set {} {} 0",  originTagId_, scoreObjectName_));
    function.emplace_back(formatString("tag {} remove {}",                originTagId_, playingTag_     ));
    function.emplace_back(formatString("scoreboard objectives remove {}", scoreObjectName_              ));
    subjoin(function, AssistFunctionGenerator::generateClearOriginFunction());

    return function;
}

VideoAssistFunctionGenerator::MCFunction VideoAssistFunctionGenerator::generateBuildFunction() const
{
    return MCFunction
    {
        formatString("execute as {} at @s run structure load \"{}-{}\" ~ ~ ~", originTagId_, structureName_, 0)
    };
}

VideoAssistFunctionGenerator::MCFunction VideoAssistFunctionGenerator::generatePlayFunction() const
{
    return MCFunction
    {
        formatString("tag {} add {}", originTagId_, playingTag_)
    };
}

VideoAssistFunctionGenerator::MCFunction VideoAssistFunctionGenerator::generatePauseFunction() const
{
    return MCFunction
    {
        formatString("tag {} remove {}", originTagId_, playingTag_)
    };
}

VideoAssistFunctionGenerator::MCFunction VideoAssistFunctionGenerator::generateStopFunction() const
{
    return MCFunction
    {
        formatString("tag {} remove {}",               originTagId_, playingTag_),
        formatString("scoreboard players set {} {} 0", originTagId_, scoreObjectName_)
    };
}

VideoAssistFunctionGenerator::MCFunction
VideoAssistFunctionGenerator::generateWorkPerTickFunction1(const std::string& function1ContentPrefix) const
{
    const int numFunctions = frameCount_ / MAX_COMMAND_COUNT_PER_MCFUNCTION + 1;

    MCFunction function;
    for (int n = 0; n < numFunctions; ++n)
    {
        const int from = n * MAX_COMMAND_COUNT_PER_MCFUNCTION;
        const int to   = from + MAX_COMMAND_COUNT_PER_MCFUNCTION - 1;
        function.emplace_back(formatString(
            "execute as {} if score @s {} matches {}..{} run function \"{}{}\"",
            playingTagId_, scoreObjectName_, from, to, function1ContentPrefix, n
        ));
    }

    return function;
}

VideoAssistFunctionGenerator::MCFunction VideoAssistFunctionGenerator::generateWorkPerTickFunction2() const
{
    return MCFunction
    {
        formatString("execute as {} if score @s {} matches 0..{} run scoreboard players add @s {} 1", playingTagId_, scoreObjectName_, frameCount_ - 1, scoreObjectName_),
        formatString("execute as {} if score @s {} matches {}.. run tag @s remove {}",                playingTagId_, scoreObjectName_, frameCount_,     playingTag_     ),
        formatString("execute as {} if score @s {} matches {}.. run scoreboard players set @s {} 0",  originTagId_,  scoreObjectName_, frameCount_,     scoreObjectName_),
    };
}

std::vector<VideoAssistFunctionGenerator::MCFunction>
VideoAssistFunctionGenerator::generateWorkPerTickFunction1Content() const
{
    std::vector<MCFunction> functions;
    const int numFunctions = frameCount_ / MAX_COMMAND_COUNT_PER_MCFUNCTION + 1;
    functions.reserve(numFunctions);

    for (int n = 0; n < numFunctions; ++n)
    {
        MCFunction function;
        for (int i = 0; i < MAX_COMMAND_COUNT_PER_MCFUNCTION; ++i)
        {
            const int index = n * MAX_COMMAND_COUNT_PER_MCFUNCTION + i;
            if (index >= frameCount_)
                break;

            function.emplace_back(formatString(
                "execute as {} at @s if score @s {} matches {} run structure load \"{}-{}\" ~ ~ ~",
                playingTagId_, scoreObjectName_, index, structureName_, index
            ));
        }

        if (!function.empty())
            functions.emplace_back(std::move(function));
    }

    return functions;
}
