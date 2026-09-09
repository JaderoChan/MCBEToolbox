#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>      // std::pair

#include "color.hpp"
#include "version.hpp"

// =============================================================================
// > 方块属性
// =============================================================================

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

/**
 * 方块属性匹配模式
 *
 * 可用于 #filterBlockDataMap()
 */
enum class BlockAttributeMatchMode
{
    ContainsAll, ///> 方块包含给定的全部属性
    Disjoint,    ///> 方块不含给定的任一属性
    SubsetOf     ///> 方块属性完全属于给定属性集合（包括无属性方块）
};

// =============================================================================
// > 方块面数据
// =============================================================================

/**
 * 目标方块面
 *
 * 可用于指定使用的方块面数据。
 */
enum class TargetSurface
{
    Up,
    Down,
    Side
};

/**
 * 方块面数据
 *
 * 存储方块各个面的材质文件路径与颜色。
 */
struct BlockSurface
{
    // {材质文件路径 : 方块颜色}
    using SurfaceData = std::pair<std::string, Rgb>;

    BlockSurface() noexcept = default;

    SurfaceData get(TargetSurface targetSurface) const
    {
        switch (targetSurface)
        {
            case TargetSurface::Up:   return up;
            case TargetSurface::Down: return down;
            case TargetSurface::Side: return side;
            default: throw std::invalid_argument("BlockSurface::get(): invalid target surface");
        }
    }

    SurfaceData up;
    SurfaceData down;
    SurfaceData side;
};

// =============================================================================
// > 方块数据
// =============================================================================

/**
 * 方块数据
 *
 * 存储游戏用方块 ID，方块属性与方块面数据。
 */
struct BlockData
{
    BlockData() noexcept = default;

    std::string     id;             ///< 游戏用方块 ID
    BlockAttributes attributes = 0; ///< 方块属性
    BlockSurface    surface;        ///< 方块面数据
};

/** 方块数据对 {程序用方块 ID : 方块数据} */
using BlockDataPair = std::pair<std::string_view, const BlockData*>;

/** 空气程序用方块 ID */
static inline const std::string   AIR_BLOCK_ID{"minecraft:air"};
/** 空气方块数据（方块各面材质均为空） */
static inline const BlockData     AIR_BLOCK_DATA{"minecraft:air", 0, BlockSurface()};
/** 空气方块数据对 */
static inline const BlockDataPair AIR_BLOCK_DATA_PAIR{AIR_BLOCK_ID, &AIR_BLOCK_DATA};

/** 方块数据映射 {程序用方块ID : 方块数据} */
using BlockDataMap  = std::map<std::string_view, const BlockData*>;

// =============================================================================
// > 方块条目
// =============================================================================

/**
 * 方块条目
 *
 * 存储方块名称，最低游戏版本与各版本方块数据。
 */
struct BlockEntry
{
    BlockEntry() noexcept = default;

    std::string                        name;             ///< 方块英文名
    Version                            minVersion;       ///< 最低可用游戏版本
    BlockData                          defaultBlockData; ///< 默认方块数据
    std::map<std::string, std::string> nameTranslations; ///< 方块译名（可以为空）{语言代码 : 译名}
    std::map<Version, BlockData>       variants;         ///< 不同版本对应的方块数据
};

/** 方块条目映射 {程序用方块ID : 方块条目} */
using BlockEntryMap = std::map<std::string, BlockEntry>;

// =============================================================================
// > 函数声明
// =============================================================================

/**
 * 从 Json 字符串中解析 #BlockEntryMap。
 *
 * @throw std::runtime_error
 * @throw std::invalid_argument
 */
BlockEntryMap parseBlockEntryMap(std::string_view json);

/** 从 #BlockEntryMap 中解析默认方块数据组成的 #BlockDataMap。 */
BlockDataMap resolveBlockEntryMap(const BlockEntryMap& blockEntryMap);

/** 从 #BlockEntryMap 中解析符合目标版本的 #BlockDataMap。 */
BlockDataMap resolveBlockEntryMap(const BlockEntryMap& blockEntryMap, Version targetVersion);

/** 根据给定方块属性与匹配模式从 #BlockDataMap 中筛选出符合规则的子集。 */
BlockDataMap filterBlockDataMap(
    const BlockDataMap&     blockDataMap,
    BlockAttributeMatchMode matchMode,
    BlockAttributes         attributes);
