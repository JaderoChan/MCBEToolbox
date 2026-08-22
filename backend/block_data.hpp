#pragma once

#include <map>
#include <string>

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
std::map<std::string, BlockData> parseBlockDatasJson(const std::string& jsonStr);

// std::map<std::string, BlockData> {Block ID : Block Data}
std::string dumpBlockDatasJson(const std::map<std::string, BlockData>& blockDatas);
