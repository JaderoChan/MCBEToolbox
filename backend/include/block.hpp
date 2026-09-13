#pragma once

#include <map>         // std::map
#include <stdexcept>   // std::invalid_argument, std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::pair

#include "color.hpp"
#include "version.hpp"

// =====================================================================================================================
// > 方块属性
// =====================================================================================================================

/**
 * 方块属性
 *
 * 可使用逻辑或运算符进行组合。
 */
enum BlockAttribute : int
{
    BLOCK_ATTRI_IS_INCOMPLETE     = 0x00000001, ///< 非完整方块
    BLOCK_ATTRI_IS_TRANSPARENT    = 0x00000002, ///< 透明/半透明方块
    BLOCK_ATTRI_IS_LUMINOUS       = 0x00000004, ///< 发光体
    BLOCK_ATTRI_IS_UNSTABLE       = 0x00000008, ///< 不稳定（会随时间或环境的影响，自发发生变化）
    BLOCK_ATTRI_IS_CREATIVE       = 0x00000010, ///< 仅创造模式可获得
    BLOCK_ATTRI_HAS_GRAVITY       = 0x00000020, ///< 具有重力
    BLOCK_ATTRI_HAS_PATTERN       = 0x00000040, ///< 具有明显花纹/纹理
    BLOCK_ATTRI_FLAMMABLE         = 0x00000080, ///< 可燃烧
    BLOCK_ATTRI_ENDERMAN_PICKABLE = 0x00000100  ///< 可被末影人拿起
};

using BlockAttributes = int;
constexpr BlockAttributes BLOCK_ATTRI_NONE = 0x00000000; ///< 不包含任何方块属性（可用于筛选 #BlockDataMap）
constexpr BlockAttributes BLOCK_ATTRI_ALL  = 0xFFFFFFFF; ///< 包含所有方块属性（可用于筛选 #BlockDataMap）

// 判断 value 是否包含 attris 指定的所有方块属性。
#define HAS_BLOCK_ATTRIS(value, attris) !!(attris == (value & attris))
// 将 attris 指定的方块属性加入 value。
#define SET_BLOCK_ATTRIS(value, attris) (value = (value | attris))
// 将 attris 指定的方块属性从 value 中移除。
#define UNSET_BLOCK_ATTRIS(value, attris) (value = (~((~value) | attris)))

/**
 * 方块属性筛选模式
 *
 * 可用于筛选 #BlockDataMap
 */
enum BlockAttributeFilterMode
{
    BLOCK_ATTRI_FILTER_MODE_CONTAINS_ALL, ///> 方块包含给定的全部属性
    BLOCK_ATTRI_FILTER_MODE_DISJOINT,     ///> 方块不含给定的任一属性
    BLOCK_ATTRI_FILTER_MODE_SUBSETOF      ///> 方块属性完全属于给定属性集合（包括无属性方块）
};

// =====================================================================================================================
// > 方块面数据
// =====================================================================================================================

/**
 * 面方向
 *
 * 可用于指定使用的方块面数据与结构体朝向。
 */
enum SurfaceDirection
{
    SURFACE_DIRECTION_UP,     ///< 使用顶面方块材质，结构体朝向为 Y 轴正方向
    SURFACE_DIRECTION_BOTTOM, ///< 使用底面方块材质，结构体朝向为 Y 轴负方向
    SURFACE_DIRECTION_NORTH,  ///< 使用侧面方块材质，结构体朝向为 Z 轴负方向
    SURFACE_DIRECTION_SOUTH,  ///< 使用侧面方块材质，结构体朝向为 Z 轴正方向
    SURFACE_DIRECTION_EAST,   ///< 使用侧面方块材质，结构体朝向为 X 轴正方向
    SURFACE_DIRECTION_WEST    ///< 使用侧面方块材质，结构体朝向为 X 轴负方向
};

/**
 * 方块面数据
 *
 * 存储方块各个面的材质文件路径与颜色。
 */
struct BlockSurfaceData
{
    // {材质文件路径 : 方块颜色}
    using TextureColorPair = std::pair<std::string, Rgb>;

    BlockSurfaceData() noexcept = default;

    TextureColorPair get(SurfaceDirection desiredSurface) const
    {
        switch (desiredSurface)
        {
            case SURFACE_DIRECTION_UP:     return up;
            case SURFACE_DIRECTION_BOTTOM: return bottom;
            case SURFACE_DIRECTION_NORTH:  // Fallthrough
            case SURFACE_DIRECTION_SOUTH:  // Fallthrough
            case SURFACE_DIRECTION_EAST:   // Fallthrough
            case SURFACE_DIRECTION_WEST:   return side;
            default: throw std::invalid_argument("BlockSurfaceData::get(): invalid surface direction");
        }
    }

    TextureColorPair up;
    TextureColorPair bottom;
    TextureColorPair side;
};

// =====================================================================================================================
// > 方块数据
// =====================================================================================================================

/**
 * 方块数据
 *
 * 存储游戏用方块 ID，方块属性与方块面数据。
 */
struct BlockData
{
    BlockData() noexcept = default;

    std::string      id;             ///< 游戏用方块 ID
    BlockAttributes  attributes = 0; ///< 方块属性
    BlockSurfaceData surfaceData;    ///< 方块面数据
};

/** 方块数据对 {程序用方块 ID : 方块数据} */
using BlockDataPair = std::pair<std::string_view, const BlockData*>;

/** 空气程序用方块 ID */
static inline const std::string   AIR_BLOCK_ID{"minecraft:air"};
/** 空气方块数据（方块各面材质均为空） */
static inline const BlockData     AIR_BLOCK_DATA{"minecraft:air", 0, BlockSurfaceData()};
/** 空气方块数据对 */
static inline const BlockDataPair AIR_BLOCK_DATA_PAIR{AIR_BLOCK_ID, &AIR_BLOCK_DATA};

/** 方块数据映射 {程序用方块ID : 方块数据} */
using BlockDataMap  = std::map<std::string_view, const BlockData*>;

// =====================================================================================================================
// > 方块条目
// =====================================================================================================================

/**
 * 方块条目
 *
 * 存储方块名称，最低游戏版本与各版本方块数据。
 */
struct BlockEntry
{
    BlockEntry() noexcept = default;

    std::string                        name;              ///< 方块英文名
    Version                            minVersion;        ///< 最低可用游戏版本
    BlockData                          baseBlock;         ///< 基础方块数据
    std::map<std::string, std::string> localizationNames; ///< 本地化方块名（可以为空）{语言代码 : 方块名}
    std::map<Version, BlockData>       versionedBlocks;   ///< 不同版本对应的方块数据
};

/** 方块条目映射 {程序用方块ID : 方块条目} */
using BlockEntryMap = std::map<std::string, BlockEntry>;

// =====================================================================================================================
// > 函数声明
// =====================================================================================================================

/**
 * 从 Json 字符串中解析 #BlockEntryMap。
 *
 * @throw std::runtime_error
 * @throw std::invalid_argument
 */
BlockEntryMap parseBlockEntries(std::string_view json);

/**
 * 从文件中解析 #BlockEntryMap。
 *
 * @throw std::runtime_error
 * @throw std::invalid_argument
 */
BlockEntryMap parseBlockEntriesFromFile(const std::string& filepath);

/** 从 #BlockEntryMap 中解析由基础方块数据组成的 #BlockDataMap。 */
BlockDataMap resolveBlockEntries(const BlockEntryMap& blockEntries);

/** 从 #BlockEntryMap 中解析符合目标版本的 #BlockDataMap。 */
BlockDataMap resolveBlockEntries(const BlockEntryMap& blockEntries, Version targetVersion);

/** 根据给定方块属性与匹配模式从 #BlockDataMap 中筛选出符合规则的子集。 */
BlockDataMap filterBlocks(const BlockDataMap& blocks, BlockAttributeFilterMode filterMode, BlockAttributes attributes);
