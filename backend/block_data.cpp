#include "block_data.hpp"

#include <assert.h>
#include <stdexcept>

#include <nlohmann/json.hpp>

#define THROW_KEY_NOT_FOUND_ERROR(_key, _type) \
throw std::runtime_error("key '" _key ":" _type "' not found")

#define CHECK_KEY_WITH_CORRECT_TYPE(_obj, _key, _type) \
do { if (!_obj.contains(_key) || _obj[_key].type() != nlohmann::json::value_t::_type) \
THROW_KEY_NOT_FOUND_ERROR(_key, #_type); } while (0)

static BlockAttributes parseBlockAttributesJson(const nlohmann::json& j) noexcept
{
    assert(j.is_object());

    BlockAttributes ba{0};
    ba |= (j.value("is_uncompeleted",    false) ? BLOCK_ATTRI_IS_UNCOMPELETED    : 0);
    ba |= (j.value("is_transparent",     false) ? BLOCK_ATTRI_IS_TRANSPARENT     : 0);
    ba |= (j.value("has_gravity",        false) ? BLOCK_ATTRI_HAS_GRAVITY        : 0);
    ba |= (j.value("can_burned",         false) ? BLOCK_ATTRI_CAN_BURNED         : 0);
    ba |= (j.value("can_enderman_taked", false) ? BLOCK_ATTRI_CAN_ENDERMAN_TAKED : 0);
    return ba;
}

static inline int hexCharToInt(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    throw std::invalid_argument("invalid hex character '" + std::string(1, c) + "'");
}

static BlockSurface::Rgb hexToRgb(std::string_view hex)
{
    if (hex.empty())
        throw std::invalid_argument("invalid hex color string '" + std::string(hex) + "'");
    if (hex[0] == '#')
        hex = hex.substr(1);

    BlockSurface::Rgb rgb;
    if (hex.size() == 3)
    {
        rgb.r = hexCharToInt(hex[0]) * 16 + hexCharToInt(hex[0]);
        rgb.g = hexCharToInt(hex[1]) * 16 + hexCharToInt(hex[1]);
        rgb.b = hexCharToInt(hex[2]) * 16 + hexCharToInt(hex[2]);
    }
    else if (hex.size() == 6)
    {
        rgb.r = hexCharToInt(hex[0]) * 16 + hexCharToInt(hex[1]);
        rgb.g = hexCharToInt(hex[2]) * 16 + hexCharToInt(hex[3]);
        rgb.b = hexCharToInt(hex[4]) * 16 + hexCharToInt(hex[5]);
    }
    else
    {
        throw std::invalid_argument("invalid hex color string '" + std::string(hex) + "'");
    }

    return rgb;
}

static BlockSurface parseBlockSurfaceJson(const nlohmann::json& j)
{
    assert(j.is_object());

    if (!j.contains("colors"))   THROW_KEY_NOT_FOUND_ERROR("colors",   "string|object");
    if (!j.contains("textures")) THROW_KEY_NOT_FOUND_ERROR("textures", "string|object");

    BlockSurface bs;
    if (j["colors"].is_string())
    {
        const BlockSurface::Rgb color = hexToRgb(j["colors"]);
        bs.colors.up    = color;
        bs.colors.down  = color;
        bs.colors.north = color;
        bs.colors.south = color;
        bs.colors.east  = color;
        bs.colors.west  = color;
        if (j["textures"].is_string())
        {
            const std::string texture = j["textures"];
            bs.textures.up    = texture;
            bs.textures.down  = texture;
            bs.textures.north = texture;
            bs.textures.south = texture;
            bs.textures.east  = texture;
            bs.textures.west  = texture;
        }
        else
        {
            throw std::runtime_error("the 'textures' field is not match to 'colors' field");
        }
    }
    else if (j["colors"].is_object())
    {
        const auto& colors   = j["colors"];
        const auto& textures = j["textures"];
        CHECK_KEY_WITH_CORRECT_TYPE(colors,   "up",   string);
        CHECK_KEY_WITH_CORRECT_TYPE(colors,   "down", string);
        CHECK_KEY_WITH_CORRECT_TYPE(textures, "up",   string);
        CHECK_KEY_WITH_CORRECT_TYPE(textures, "down", string);
        bs.colors.up      = hexToRgb(j["colors"]["up"]);
        bs.colors.down    = hexToRgb(j["colors"]["down"]);
        bs.textures.up    = j["textures"]["up"];
        bs.textures.down  = j["textures"]["down"];

        if (colors.contains("side"))
        {
            CHECK_KEY_WITH_CORRECT_TYPE(colors,   "side", string);
            CHECK_KEY_WITH_CORRECT_TYPE(textures, "side", string);

            const BlockSurface::Rgb color = hexToRgb(j["colors"]["side"]);
            bs.colors.north   = color;
            bs.colors.south   = color;
            bs.colors.east    = color;
            bs.colors.west    = color;
            const std::string texture = j["textures"]["side"];
            bs.textures.north = texture;
            bs.textures.south = texture;
            bs.textures.east  = texture;
            bs.textures.west  = texture;
        }
        else
        {
            CHECK_KEY_WITH_CORRECT_TYPE(colors,   "north", string);
            CHECK_KEY_WITH_CORRECT_TYPE(colors,   "south", string);
            CHECK_KEY_WITH_CORRECT_TYPE(colors,   "east",  string);
            CHECK_KEY_WITH_CORRECT_TYPE(colors,   "west",  string);
            CHECK_KEY_WITH_CORRECT_TYPE(textures, "north", string);
            CHECK_KEY_WITH_CORRECT_TYPE(textures, "south", string);
            CHECK_KEY_WITH_CORRECT_TYPE(textures, "east",  string);
            CHECK_KEY_WITH_CORRECT_TYPE(textures, "west",  string);

            bs.colors.north   = hexToRgb(j["colors"]["north"]);
            bs.colors.south   = hexToRgb(j["colors"]["south"]);
            bs.colors.east    = hexToRgb(j["colors"]["east"]);
            bs.colors.west    = hexToRgb(j["colors"]["west"]);
            bs.textures.north = j["textures"]["north"];
            bs.textures.south = j["textures"]["south"];
            bs.textures.east  = j["textures"]["east"];
            bs.textures.west  = j["textures"]["west"];
        }
    }
    else
    {
        throw std::runtime_error("the 'colors' field is neither 'string' type nor 'object' type");
    }
    return bs;
}

static BlockDataDetail parseBlockDataDetailJson(const nlohmann::json& j)
{
    assert(j.is_object());

    BlockDataDetail bdd{};
    CHECK_KEY_WITH_CORRECT_TYPE(j, "structure_id", string);
    CHECK_KEY_WITH_CORRECT_TYPE(j, "command_id",   string);
    bdd.structureId = j["structure_id"];
    bdd.commandId   = j["command_id"];
    // The 'attributes' field is optional.
    bdd.attributes  = 0;
    if (j.contains("attributes"))
    {
        CHECK_KEY_WITH_CORRECT_TYPE(j, "attributes", object);
        bdd.attributes = parseBlockAttributesJson(j["attributes"]);
    }
    bdd.surface     = std::move(parseBlockSurfaceJson(j));
    return bdd;
}

static BlockDatas parseBlockDatasJsonHelper(std::string_view json)
{
    const nlohmann::json j = nlohmann::json::parse(json, nullptr, true, true);
    if (j.is_discarded() || !j.is_object())
        throw std::runtime_error("illegal json data or root item is not 'object' type");

    BlockDatas bds;
    for (const auto& [key, value] : j.items())
    {
        // Skip item which is not 'object' type.
        if (!value.is_object()) continue;

        BlockData bd{};
        // Parse 'name' and 'joined_version' fileds.
        CHECK_KEY_WITH_CORRECT_TYPE(value, "name",            string);
        CHECK_KEY_WITH_CORRECT_TYPE(value, "joined_version",  string);
        CHECK_KEY_WITH_CORRECT_TYPE(value, "default_texture", string);
        bd.name           = value["name"];
        bd.joinedVersion  = parseVersionString(value["joined_version"]);
        bd.defaultTexture = value["default_texture"];

        // Parse 'name_translations' filed, it is optional.
        if (value.contains("name_translations"))
        {
            CHECK_KEY_WITH_CORRECT_TYPE(value, "name_translations", object);
            for (const auto& [langCode, trans] : value["name_translations"].items())
            {
                if (!trans.is_string())
                    throw std::runtime_error("name translation item is not a string");
                bd.nameTranslations[langCode] = trans;
            }
        }

        // Parse 'datas' filed.
        CHECK_KEY_WITH_CORRECT_TYPE(value, "datas", object);
        for (const auto& [versStr, data] : value["datas"].items())
        {
            const Version   vers = parseVersionString(versStr);
            BlockDataDetail bdd  = std::move(parseBlockDataDetailJson(data));
            bd.datas[vers] = std::move(bdd);
        }

        if (!bd.datas.empty())
            bds[key] = std::move(bd);
    }
    return bds;
}

BlockDatas parseBlockDatasJson(std::string_view json)
{
    try { return parseBlockDatasJsonHelper(json); }
    catch (std::runtime_error& e)
    { throw std::runtime_error("Invalid 'block data' json, " + std::string(e.what())); }
}

BlockDataDetails filterBlockDatas(
    const BlockDatas& blockDatas,
    Version           targetVersion)
{
    BlockDataDetails bdds;
    for (const auto& [id, bd] : blockDatas)
    {
        if (bd.joinedVersion >= targetVersion)
        {
            for (const auto& [vers, bdd] : bd.datas)
            {
                if (vers >= targetVersion)
                {
                    bdds[id] = bdd;
                    break;
                }
            }
        }
    }
    return bdds;
}

BlockDataDetails filterBlockDataDetails(
    const BlockDataDetails& blockDataDetails,
    BlockAttributes         targetAttributes)
{
    BlockDataDetails bdds;
    for (const auto& [id, bdd] : blockDataDetails)
    {
        if (bdd.attributes == (bdd.attributes & targetAttributes))
            bdds[id] = bdd;
    }
    return bdds;
}

BlockDataDetails filterBlockDataDetails(
    const BlockDataDetails&          blockDataDetails,
    std::unordered_set<std::string>& excludes)
{
    BlockDataDetails bdds;
    for (const auto& [id, bdd] : blockDataDetails)
    {
        if (excludes.count(id) == 0)
            bdds[id] = bdd;
    }
    return bdds;
}

#undef THROW_KEY_NOT_FOUND_ERROR
