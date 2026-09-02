#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "color.hpp"
#include "version.hpp"

/** 方块属性，可使用逻辑或运算符进行组合。 */
enum BlockAttribute : int
{
    BLOCK_ATTRI_IS_INCOMPLETE     = 0x00000001,
    BLOCK_ATTRI_IS_TRANSPARENT    = 0x00000002,
    BLOCK_ATTRI_HAS_GRAVITY       = 0x00000004,
    BLOCK_ATTRI_CAN_BURN          = 0x00000008,
    BLOCK_ATTRI_ENDERMAN_PICKABLE = 0x00000010
};
using BlockAttributes = int;
/** 不具有任何方块属性 */
constexpr BlockAttributes BLOCK_ATTRI_NONE = 0x00000000;
/** 具有所有方块属性 */
constexpr BlockAttributes BLOCK_ATTRI_ALL  = 0xFFFFFFFF;

/** 描述方块各个面的材质文件路径与颜色。 */
struct BlockSurface
{
    BlockSurface() = default;

    // 便利函数：同时赋值所有面数据
    void assignAllSurfaces(const std::pair<std::string, Rgb>& surface)
    {
        up    = surface;
        down  = surface;
        north = surface;
        south = surface;
        east  = surface;
        west  = surface;
    }

    // 便利函数：同时赋值所有侧面数据
    void assignSideSurfaces(const std::pair<std::string, Rgb>& sideSurface)
    {
        north = sideSurface;
        south = sideSurface;
        east  = sideSurface;
        west  = sideSurface;
    }

    // {材质文件路径 : 方块颜色}
    std::pair<std::string, Rgb> up;
    std::pair<std::string, Rgb> down;
    std::pair<std::string, Rgb> north;
    std::pair<std::string, Rgb> south;
    std::pair<std::string, Rgb> east;
    std::pair<std::string, Rgb> west;
};

/** 方块数据 */
struct BlockData
{
    BlockData() : attributes(0) {}

    BlockAttributes attributes;     // 方块属性
    std::string     structureNbtId; // 方块在 NBT 结构文件中的 ID
    std::string     commandId;      // 方块在指令中的 ID
    BlockSurface    surface;        // 方块面数据
};

/** 方块条目 */
struct BlockEntry
{
    BlockEntry() = default;

    std::string name;               // 方块英文名
    Version     addedVersion;       // 加入游戏的版本
    BlockData   defaultBlockData;   // 默认方块数据
    std::unordered_map<std::string, std::string> nameTranslations; // {语言代码 : 方块译名}（可以为空）
    std::unordered_map<Version, BlockData>       variants;         // 不同版本对应的方块数据
};

// {方块ID : 方块条目}
using BlockEntryMap = std::unordered_map<const std::string*, const BlockEntry*>;
// {方块ID : 方块数据}
using BlockDataMap  = std::unordered_map<const std::string*, const BlockData*>;

/**
 * 从 Json 字符串中解析 BlockEntryMap。
 *
 * @note BlockEntryMap 中的数据需要通过 \ref releaseBlockEntryMap() 进行释放。
 * 释放之后，通过此 BlockEntryMap 所获得的 BlockDataMap 等数据也不再有效。
 *
 * @throw std::runtime_error
 * @throw std::invalid_argument
 */
BlockEntryMap parseBlockEntryMapFromJson(std::string_view json);

/**
 * 释放 BlockEntryMap 中的数据。
 */
void releaseBlockEntryMap(BlockEntryMap& blockEntryMap);

/**
 * 从 BlockEntryMap 中解析默认方块数据组成的 BlockDataMap。
 */
BlockDataMap resolveBlockEntryMap(const BlockEntryMap& blockEntryMap);

/**
 * 从 BlockEntryMap 中解析符合目标版本的 BlockDataMap。
 */
BlockDataMap resolveBlockEntryMap(const BlockEntryMap& blockEntryMap, Version targetVersion);

/**
 * 从 BlockDataMap 中筛选出符合方块属性规则的子集。
 */
BlockDataMap filterBlockAttributes(const BlockDataMap& blockDataMap, BlockAttributes attributes);
