#pragma once

#include <map>
#include <string>
#include <string_view>
#include <unordered_set>

#include "version.hpp"
#include "block_attribute.hpp"
#include "block_surface.hpp"

struct BlockDataDetail
{
    BlockAttributes attributes;
    std::string     structureId;
    std::string     commandId;
    BlockSurface    surface;
};

struct BlockData
{
    Version                            joinedVersion;
    std::string                        name;             // English name
    std::string                        defaultTexture;   // Used to 'block image' and 'block icon' UI
    Rgb                                defaultColor;     // Used to 'block image'
    std::map<std::string, std::string> nameTranslations; // {Language code : Name} Canbe empty
    std::map<Version, BlockDataDetail> datas;
};

// {Block ID : Block Data Detail}
using BlockDataDetails = std::map<std::string, BlockDataDetail>;
// {Block ID : Block Data}
using BlockDatas       = std::map<std::string, BlockData>;

/**
 * @throw std::runtime_error
 * @throw std::invalid_argument
 */
BlockDatas parseBlockDatasJson(std::string_view json);

BlockDataDetails filterBlockDatas(
    const BlockDatas& blockDatas,
    Version           targetVersion);

BlockDataDetails filterBlockDataDetails(
    const BlockDataDetails& blockDataDetails,
    BlockAttributes         targetAttributes);

template<typename V>
std::map<std::string, V> excludeKeys(
    const std::map<std::string, V>&        map,
    const std::unordered_set<std::string>& keys)
{
    std::map<std::string, V> ret;
    for (const auto& [k, v] : map)
    {
        if (keys.count(k) == 0)
            ret[k] = v;
    }
    return ret;
}
