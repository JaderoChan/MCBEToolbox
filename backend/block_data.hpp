#pragma once

#include <map>
#include <string>
#include <string_view>

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
    std::map<std::string, std::string> nameTranslations; // {Language code : Name} Canbe empty
    std::map<Version, BlockDataDetail> datas;
};

// std::map<std::string, BlockData> {Block ID : Block Data}
using BlockDatas = std::map<std::string, BlockData>;

/** @throw std::runtime_error if parsing fails */
BlockDatas parseBlockDatasJson(std::string_view json);
