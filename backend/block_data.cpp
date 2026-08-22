#include "block_data.hpp"

#include <stdexcept>

#include <nlohmann/json.hpp>

static BlockAttributes parseBlockAttributesJson(const nlohmann::json& j)
{
    BlockAttributes attributes{0};
    attributes |= (j.value("is_uncompeleted",    false) ? BLOCK_ATTRI_IS_UNCOMPELETED    : 0);
    attributes |= (j.value("is_transparent",     false) ? BLOCK_ATTRI_IS_TRANSPARENT     : 0);
    attributes |= (j.value("has_gravity",        false) ? BLOCK_ATTRI_HAS_GRAVITY        : 0);
    attributes |= (j.value("can_burned",         false) ? BLOCK_ATTRI_CAN_BURNED         : 0);
    attributes |= (j.value("can_enderman_taked", false) ? BLOCK_ATTRI_CAN_ENDERMAN_TAKED : 0);
    return attributes;
}

static BlockSurface parseBlockSurfaceJson(const nlohmann::json& j)
{
    if (j.at("colors").is_string())
    {

    }
    else if (j.at("color").is_object())
    {

    }
    else
    {

    }
}

static BlockDataDetail parseBlockDataDetailJson(const nlohmann::json& j)
{
    BlockDataDetail bdd{};
    bdd.structureId = j.at("structure_id");
    bdd.commandId   = j.at("command_id");
    bdd.attributes  = parseBlockAttributesJson(j.at("attributes"));
    bdd.surface     = std::move(parseBlockSurfaceJson(j));
    return bdd;
}

std::map<std::string, BlockData> parseBlockDatasJson(const std::string& jsonStr)
{
    using namespace nlohmann;

    const json j = json::parse(jsonStr);
    std::map<std::string, BlockData> ret;
    for (const auto& [key, value] : j.items())
    {
        BlockData bd{};
        bd.name          = value.at("name");
        bd.joinedVersion = parseVersionString(value.at("joined_version"));
        // Name translations is optional.
        if (value.contains("nameTranslations"))
        {
            for (const auto& [langCode, trans] : value.at("nameTranslations").items())
                bd.nameTranslations[langCode] = trans;
        }
        for (const auto& [versionStr, data] : value.at("datas").items())
        {
            Version         version = parseVersionString(versionStr);
            BlockDataDetail bdd     = std::move(parseBlockDataDetailJson(data));
            bd.datas[version] = std::move(bdd);
        }
        ret[key] = std::move(bd);
    }
    return ret;
}
