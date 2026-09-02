#include "block.hpp"

#include <assert.h>
#include <algorithm>
#include <stdexcept>
#include <vector>

#include <nlohmann/json.hpp>

namespace
{

// 抛出 “键不存在” 异常
#define THROW_KEY_NOT_FOUND_ERROR(obj_name, key) \
throw std::runtime_error(std::string("key '") + key + "' not found in object '" + obj_name + "'")

// 抛出 “键类型不正确” 异常
#define THROW_KEY_UNCORRECT_TYPE_ERROR(obj_name, key, got_type, expected_type)  \
throw std::runtime_error(                                                       \
    std::string("key '") + key + "' expected type is '" #expected_type          \
    "' but got '" + got_type + "' type in object '" + obj_name + "'"            \
)

// 检查指定对象中是否存在符合预期类型的键
#define CHECK_KEY(obj, obj_name, key, expected_type)                            \
do {                                                                            \
    if (!obj.contains(key))                                                     \
        THROW_KEY_NOT_FOUND_ERROR(obj_name, key);                               \
    if (obj[key].type() != nlohmann::json::value_t::expected_type)              \
        THROW_KEY_UNCORRECT_TYPE_ERROR(                                         \
            obj_name, key, obj[key].type_name(), expected_type);                \
} while (0)

// 从给定 json 对象的字段中读取颜色值
inline Rgb readRgb(const nlohmann::json& obj, const char* field)
{
    assert(!obj.is_discarded() && obj.is_object());
    assert(obj.contains(field) && obj[field].is_string());

    const std::string s = obj[field];
    return Rgb::fromHex(s);
}

// 从给定 json 对象的字段中读取版本号
inline Version readVersion(const nlohmann::json& obj, const char* field)
{
    assert(!obj.is_discarded() && obj.is_object());
    assert(obj.contains(field) && obj[field].is_string());

    const std::string s = obj[field];
    return Version::fromString(s);
}

// 解析方块属性
void parseBlockAttributes(const nlohmann::json& obj, BlockAttributes& attributes) noexcept
{
    assert(!obj.is_discarded() && obj.is_object());

// 将 value 指定位，置为 1
#define BITS_TO_ONE(value,  bits) (value | bits)
// 将 value 指定位，置为 0
#define BITS_TO_ZERO(value, bits) (~((~value) | bits))
// 尝试读取新的属性值并覆盖原有值
#define TRY_OVERRIDE_ATTRIBUTE(attribute_name, attribute_value)                 \
do {                                                                            \
    if (obj.contains(attribute_name) && obj[attribute_name].is_boolean())       \
        attributes = (                                                          \
            obj[attribute_name]                                                 \
            ? BITS_TO_ONE(attributes,  attribute_value)                         \
            : BITS_TO_ZERO(attributes, attribute_value));                       \
} while(0);

    TRY_OVERRIDE_ATTRIBUTE("is_incomplete",     BLOCK_ATTRI_IS_INCOMPLETE);
    TRY_OVERRIDE_ATTRIBUTE("is_transparent",    BLOCK_ATTRI_IS_TRANSPARENT);
    TRY_OVERRIDE_ATTRIBUTE("is_luminous",       BLOCK_ATTRI_IS_LUMINOUS);
    TRY_OVERRIDE_ATTRIBUTE("is_unstable",       BLOCK_ATTRI_IS_UNSTABLE);
    TRY_OVERRIDE_ATTRIBUTE("is_creative",       BLOCK_ATTRI_IS_CREATIVE);
    TRY_OVERRIDE_ATTRIBUTE("has_gravity",       BLOCK_ATTRI_HAS_GRAVITY);
    TRY_OVERRIDE_ATTRIBUTE("has_pattern",       BLOCK_ATTRI_HAS_PATTERN);
    TRY_OVERRIDE_ATTRIBUTE("flammable",         BLOCK_ATTRI_FLAMMABLE);
    TRY_OVERRIDE_ATTRIBUTE("enderman_pickable", BLOCK_ATTRI_ENDERMAN_PICKABLE);

#undef TRY_OVERRIDE_ATTRIBUTE
#undef BITS_TO_ZERO
#undef BITS_TO_ONE
}

// 解析方块面数据
// 如果 force 为真，则要求对象必须包含所有面信息。
void parseBlockSurface(const nlohmann::json& obj, const char* objName, BlockData& data, bool force)
{
    assert(!obj.is_discarded() && obj.is_object());

    auto& surface = data.surface;
    if (obj["textures"].is_string())
    {
        if (!obj["colors"].is_string())
            throw std::runtime_error(
                std::string("key 'colors' type is not match to 'textures' type in object '") +
                objName + "'"
            );
        const Rgb rgb = readRgb(obj, "colors");
        surface.assignAllSurfaces({obj["textures"], rgb});
    }
    else if (obj["textures"].is_object())
    {
        if (!obj["colors"].is_object())
            throw std::runtime_error(
                std::string("key 'colors' type is not match to 'textures' type in object '") +
                objName + "'"
            );

        const auto& texturesObj = obj["textures"];
        const auto& colorsObj   = obj["colors"];

    // 抛出 colors 和 textures 模式不匹配异常
    //（如 colors 指定 side 面，而 textures 指定了所有面；或者 colors 缺失 textures 对应的键）
    #define THROW_PATTERN_NOT_MATCH(obj_name)                                           \
    throw std::runtime_error(std::string(                                               \
            "key 'colors' pattern is not match to 'textures' pattern in object '") +    \
            objName + "'"                                                               \
        );

        if (force)
        {
            CHECK_KEY(texturesObj, "textures", "up",   string);
            CHECK_KEY(texturesObj, "textures", "down", string);
            CHECK_KEY(colorsObj,   "colors",   "up",   string);
            CHECK_KEY(colorsObj,   "colors",   "down", string);

            surface.up   = {texturesObj["up"],   readRgb(colorsObj, "up")};
            surface.down = {texturesObj["down"], readRgb(colorsObj, "down")};

            if (texturesObj.contains("side") && !colorsObj.contains("side"))
                THROW_PATTERN_NOT_MATCH(objName);

            if (texturesObj.contains("side"))
            {
                CHECK_KEY(texturesObj, "textures", "side", string);
                CHECK_KEY(colorsObj,   "colors",   "side", string);
                surface.assignSideSurfaces({texturesObj["side"], readRgb(colorsObj, "side")});
            }
            else
            {
                CHECK_KEY(texturesObj, "textures", "north", string);
                CHECK_KEY(texturesObj, "textures", "south", string);
                CHECK_KEY(texturesObj, "textures", "east",  string);
                CHECK_KEY(texturesObj, "textures", "west",  string);
                CHECK_KEY(colorsObj,   "colors",   "north", string);
                CHECK_KEY(colorsObj,   "colors",   "south", string);
                CHECK_KEY(colorsObj,   "colors",   "east",  string);
                CHECK_KEY(colorsObj,   "colors",   "west",  string);
                surface.north = {texturesObj["north"], readRgb(colorsObj, "north")};
                surface.south = {texturesObj["south"], readRgb(colorsObj, "south")};
                surface.east  = {texturesObj["east"],  readRgb(colorsObj, "east")};
                surface.west  = {texturesObj["west"],  readRgb(colorsObj, "west")};
            }
        }
        else
        {
            if (texturesObj.contains("up"))
            {
                if (!colorsObj.contains("up")) THROW_PATTERN_NOT_MATCH(objName);
                surface.up = {texturesObj["up"], readRgb(colorsObj, "up")};
            }
            if (texturesObj.contains("down"))
            {
                if (!colorsObj.contains("down")) THROW_PATTERN_NOT_MATCH(objName);
                surface.down = {texturesObj["down"], readRgb(colorsObj, "down")};
            }
            if (texturesObj.contains("north"))
            {
                if (!colorsObj.contains("north")) THROW_PATTERN_NOT_MATCH(objName);
                surface.north = {texturesObj["north"], readRgb(colorsObj, "north")};
            }
            if (texturesObj.contains("south"))
            {
                if (!colorsObj.contains("south")) THROW_PATTERN_NOT_MATCH(objName);
                surface.south = {texturesObj["south"], readRgb(colorsObj, "south")};
            }
            if (texturesObj.contains("east"))
            {
                if (!colorsObj.contains("east")) THROW_PATTERN_NOT_MATCH(objName);
                surface.east = {texturesObj["east"], readRgb(colorsObj, "east")};
            }
            if (texturesObj.contains("west"))
            {
                if (!colorsObj.contains("west")) THROW_PATTERN_NOT_MATCH(objName);
                surface.west = {texturesObj["west"], readRgb(colorsObj, "west")};
            }
        }
    }
    else
    {
        THROW_KEY_UNCORRECT_TYPE_ERROR(
            objName, "textures", obj["textures"].type_name(), string|object
        );
    }

    #undef THROW_PATTERN_NOT_MATCH
}

// 解析方块数据
// 如果 force 为真，则要求对象必须包含 structure_nbt_id，command_id 和所有面数据
void parseBlockData(const nlohmann::json& obj, const char* objName, BlockData& data, bool force)
{
    assert(!obj.is_discarded() && obj.is_object());

    if (force)
    {
        if (!obj.contains("structure_nbt_id") || !obj.contains("command_id") ||
            !obj.contains("textures")         || !obj.contains("colors"))
        {
            throw std::runtime_error(
                "missing necessary fields (structure_nbt_id|command_id|textures|colors) for forced "
                "Block Data"
            );
        }
    }

    if (obj.contains("structure_nbt_id"))
    {
        CHECK_KEY(obj, objName, "structure_nbt_id", string);
        data.structureNbtId = obj["structure_nbt_id"];
    }
    if (obj.contains("command_id"))
    {
        CHECK_KEY(obj, objName, "command_id", string);
        data.commandId = obj["command_id"];
    }
    if (obj.contains("attributes"))
    {
        CHECK_KEY(obj, objName, "attributes", object);
        parseBlockAttributes(obj["attributes"], data.attributes);
    }
    if (obj.contains("textures") && obj.contains("colors"))
    {
        parseBlockSurface(obj, objName, data, force);
    }
}

void parseBlockEntryMapFromJsonHelper(std::string_view json, BlockEntryMap& blockEntryMap)
{
    const nlohmann::json j = nlohmann::json::parse(json, nullptr, true, true);
    if (j.is_discarded() || !j.is_object())
        throw std::runtime_error("illegal json data or root item is not 'object' type");

    for (const auto& [k, v] : j.items())
    {
        // 如果不是 object 类型的字段直接跳过。
        if (!v.is_object()) continue;

        BlockEntry entry;

        // 读取 name 和 added_version 字段。
        CHECK_KEY(v, k, "name", string);
        entry.name = v["name"];
        if (v.contains("added_version"))
        {
            CHECK_KEY(v, k, "added_version", string);
            entry.addedVersion = readVersion(v, "added_version");
        }

        // 解析 name_translations 字段。
        if (v.contains("name_translations"))
        {
            CHECK_KEY(v, k, "name_translations", object);
            const auto& nameTransObj = v["name_translations"];
            for (const auto& [lang, trans] : nameTransObj.items())
            {
                CHECK_KEY(nameTransObj, "name_translations", lang, string);
                entry.nameTranslations[lang] = trans;
            }
        }

        // 解析 default 方块数据
        CHECK_KEY(v, k, "default", object);
        const auto& defaultObj = v["default"];
        parseBlockData(defaultObj, "default", entry.defaultBlockData, true);

        // 解析不同版本的方块数据
        if (v.contains(("variants")))
        {
            CHECK_KEY(v, k, "variants", object);
            const auto& variantsObj = v["variants"];

            // 按照版本号对变体数据进行排序，用于实现 “新版本数据继承上一版本数据” 的功能
            std::vector<std::pair<Version, const nlohmann::json*>> sortedVariants;
            for (const auto& [versionStr, dataObj] : variantsObj.items())
            {
                if (dataObj.empty()) continue;
                CHECK_KEY(variantsObj, "variants", versionStr, object);
                const Version version = Version::fromString(versionStr);
                sortedVariants.emplace_back(version, &dataObj);
            }

            std::sort(
                sortedVariants.begin(),
                sortedVariants.end(),
                [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

            // 实际解析行为
            BlockData lastData = entry.defaultBlockData;
            for (std::size_t i = 0; i < sortedVariants.size(); ++i)
            {
                const auto& [version, dataObj] = sortedVariants[i];
                // 如果出现相同版本的方块数据，抛出异常
                if (i > 0 && version == sortedVariants[i - 1].first)
                    throw std::runtime_error("duplicate variant version '" + version.toString() + "'");

                BlockData data = lastData;
                parseBlockData(*dataObj, version.toString().c_str(), data, false);
                entry.variants[version] = std::move(data);
                lastData = entry.variants.at(version);
            }
        }

        const std::string* blockId    = new std::string(k);
        const BlockEntry*  blockEntry = new BlockEntry(std::move(entry));
        blockEntryMap[blockId] = blockEntry;
    }
}

#undef CHECK_KEY
#undef THROW_KEY_UNCORRECT_TYPE_ERROR
#undef THROW_KEY_NOT_FOUND_ERROR

} // namespace

BlockEntryMap parseBlockEntryMapFromJson(std::string_view json)
{
    // 包装一层异常消息并处理异常抛出时资源的释放
    BlockEntryMap ret;
    try
    {
        parseBlockEntryMapFromJsonHelper(json, ret);
    }
    catch (std::exception& e)
    {
        releaseBlockEntryMap(ret);
        throw std::runtime_error(
            "invalid json for parse 'Block Entry Map': " + std::string(e.what())
        );
    }
    return ret;
}

void releaseBlockEntryMap(BlockEntryMap& blockEntryMap)
{
    for (const auto& [id, entry] : blockEntryMap)
    {
        delete id;
        delete entry;
    }
}

BlockDataMap resolveBlockEntryMap(const BlockEntryMap& blockEntryMap)
{
    BlockDataMap ret;
    for (const auto& [id, entry] : blockEntryMap)
        ret[id] = &entry->defaultBlockData;
    return ret;
}

BlockDataMap resolveBlockEntryMap(const BlockEntryMap& blockEntryMap, Version targetVersion)
{
    BlockDataMap ret;
    for (const auto& [id, entry] : blockEntryMap)
    {
        // 如果加入版本比目标版本更新，说明在目标版本中此方块还未被加入，直接跳过。
        if (entry->addedVersion > targetVersion)
            continue;

        // 目标是获取不超过 targetVersion 的最新版本的方块数据。
        Version lastestVersion(0, 0, 0);
        const BlockData* lastestData = &entry->defaultBlockData;
        for (const auto& [version, data] : entry->variants)
        {
            if (version > targetVersion)
                continue;
            if (version > lastestVersion)
            {
                lastestVersion = version;
                lastestData    = &data;
            }
        }

        ret[id] = lastestData;
    }
    return ret;
}

BlockDataMap filterBlockAttributes(const BlockDataMap& blockDataMap, BlockAttributes attributes)
{
    BlockDataMap ret;
    for (const auto& [id, data] : blockDataMap)
    {
        if (data->attributes == (data->attributes & attributes))
            ret[id] = data;
    }
    return ret;
}
