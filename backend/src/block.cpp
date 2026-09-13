#include <block.hpp>

#include <assert.h> // assert
#include <fstream>  // std::ifstream

#include <nlohmann/json.hpp> // nlohmann::*

namespace
{

// 抛出 “键不存在” 异常
#define THROW_KEY_NOT_FOUND_ERROR(obj_name, key)                                                    \
throw std::runtime_error(std::string("key '") + key + "' not found in object '" + obj_name + "'")

// 抛出 “键类型不正确” 异常
#define THROW_KEY_UNCORRECT_TYPE_ERROR(obj_name, key, got_type, expected_type)                      \
throw std::runtime_error(                                                                           \
    std::string("key '") + key + "' expected type is '" #expected_type                              \
    "' but got '" + got_type + "' type in object '" + obj_name + "'"                                \
)

// 检查指定对象中是否存在符合预期类型的键
#define CHECK_KEY(obj, obj_name, key, expected_type)                                                \
do {                                                                                                \
    if (!obj.contains(key))                                                                         \
        THROW_KEY_NOT_FOUND_ERROR(obj_name, key);                                                   \
    if (obj[key].type() != nlohmann::json::value_t::expected_type)                                  \
        THROW_KEY_UNCORRECT_TYPE_ERROR(obj_name, key, obj[key].type_name(), expected_type);         \
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

// 尝试读取新的属性值并覆盖原有值
#define TRY_OVERRIDE_ATTRIBUTE(attribute_name, attribute_value)                 \
do {                                                                            \
    if (obj.contains(attribute_name) && obj[attribute_name].is_boolean())       \
        obj[attribute_name]                                                     \
        ? SET_BLOCK_ATTRIS(attributes,  attribute_value)                        \
        : UNSET_BLOCK_ATTRIS(attributes, attribute_value);                      \
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
}

// 解析方块面数据
// 如果 force 为真，则要求对象必须包含所有面信息。
void parseBlockSurfaceData(const nlohmann::json& obj, const char* objName, BlockData& block, bool force)
{
    assert(!obj.is_discarded() && obj.is_object());

    auto& surfaceData = block.surfaceData;
    if (obj["textures"].is_string())
    {
        if (!obj["colors"].is_string())
            throw std::runtime_error(
                std::string("key 'colors' type is not match to 'textures' type in object '") + objName + "'"
            );

        const Rgb rgb = readRgb(obj, "colors");
        const std::pair<std::string, Rgb> value = {obj["textures"], rgb};
        surfaceData.up     = value;
        surfaceData.bottom = value;
        surfaceData.side   = value;
    }
    else if (obj["textures"].is_object())
    {
        if (!obj["colors"].is_object())
            throw std::runtime_error(
                std::string("key 'colors' type is not match to 'textures' type in object '") + objName + "'"
            );

        const auto& texturesObj = obj["textures"];
        const auto& colorsObj   = obj["colors"];

    // 抛出 colors 和 textures 模式不匹配异常
    //（如 colors 指定 side 面，而 textures 指定了所有面；或者 colors 缺失 textures 对应的键）
    #define THROW_PATTERN_NOT_MATCH(obj_name)                                                       \
    throw std::runtime_error(std::string(                                                           \
        "key 'colors' pattern is not match to 'textures' pattern in object '") + objName + "'"      \
    );

        if (force)
        {
            CHECK_KEY(texturesObj, "textures", "up",     string);
            CHECK_KEY(texturesObj, "textures", "bottom", string);
            CHECK_KEY(texturesObj, "textures", "side",   string);
            CHECK_KEY(colorsObj,   "colors",   "up",     string);
            CHECK_KEY(colorsObj,   "colors",   "bottom", string);
            CHECK_KEY(colorsObj,   "colors",   "side",   string);

            surfaceData.up     = {texturesObj["up"],     readRgb(colorsObj, "up")};
            surfaceData.bottom = {texturesObj["bottom"], readRgb(colorsObj, "bottom")};
            surfaceData.side   = {texturesObj["side"],   readRgb(colorsObj, "side")};
        }
        else
        {
            if (texturesObj.contains("up"))
            {
                if (!colorsObj.contains("up")) THROW_PATTERN_NOT_MATCH(objName);
                surfaceData.up = {texturesObj["up"], readRgb(colorsObj, "up")};
            }
            if (texturesObj.contains("bottom"))
            {
                if (!colorsObj.contains("bottom")) THROW_PATTERN_NOT_MATCH(objName);
                surfaceData.bottom = {texturesObj["bottom"], readRgb(colorsObj, "bottom")};
            }
            if (texturesObj.contains("side"))
            {
                if (!colorsObj.contains("side")) THROW_PATTERN_NOT_MATCH(objName);
                surfaceData.side = {texturesObj["side"], readRgb(colorsObj, "side")};
            }
        }
    }
    else
    {
        THROW_KEY_UNCORRECT_TYPE_ERROR(objName, "textures", obj["textures"].type_name(), string|object);
    }

    #undef THROW_PATTERN_NOT_MATCH
}

// 解析方块数据
// 如果 force 为真，则要求对象必须包含 id 和所有面数据
void parseBlockData(const nlohmann::json& obj, const char* objName, BlockData& block, bool force)
{
    assert(!obj.is_discarded() && obj.is_object());

    if (force)
    {
        if (!obj.contains("id") || !obj.contains("textures") || !obj.contains("colors"))
            throw std::runtime_error("missing necessary fields (id|textures|colors) for forced Block Data");
    }

    if (obj.contains("id"))
    {
        CHECK_KEY(obj, objName, "id", string);
        block.id = obj["id"];
    }
    if (obj.contains("attributes"))
    {
        CHECK_KEY(obj, objName, "attributes", object);
        parseBlockAttributes(obj["attributes"], block.attributes);
    }
    if (obj.contains("textures") && obj.contains("colors"))
    {
        parseBlockSurfaceData(obj, objName, block, force);
    }
}

void parseBlockEntriesHelper(std::string_view json, BlockEntryMap& blockEntries)
{
    const nlohmann::json j = nlohmann::json::parse(json, nullptr, true, true);
    if (j.is_discarded() || !j.is_object())
        throw std::runtime_error("illegal json data or root item is not 'object' type");

    for (const auto& [id, entryObj] : j.items())
    {
        // 如果不是 object 类型的字段直接跳过。
        if (!entryObj.is_object()) continue;

        BlockEntry entry;

        // 读取 name 和 min_version 字段。
        CHECK_KEY(entryObj, id, "name", string);
        entry.name = entryObj["name"];
        if (entryObj.contains("min_version"))
        {
            CHECK_KEY(entryObj, id, "min_version", string);
            entry.minVersion = readVersion(entryObj, "min_version");
        }
        else
        {
            // 默认最低版本：1.20.50.7（苍园觉醒版本）
            constexpr Version defaultVersion(1, 21, 50, 7);
            entry.minVersion = defaultVersion;
        }

        // 解析 localization_names 字段。
        if (entryObj.contains("localization_names"))
        {
            CHECK_KEY(entryObj, id, "localization_names", object);
            const auto& localizationNamesObj = entryObj["localization_names"];
            for (const auto& [locale, name] : localizationNamesObj.items())
            {
                CHECK_KEY(localizationNamesObj, "localization_names", locale, string);
                entry.localizationNames[locale] = name;
            }
        }

        // 解析 base 方块数据
        CHECK_KEY(entryObj, id, "base", object);
        const auto& baseBlockObj = entryObj["base"];
        parseBlockData(baseBlockObj, "base", entry.baseBlock, true);

        // 解析不同版本的方块数据
        if (entryObj.contains(("versioned")))
        {
            CHECK_KEY(entryObj, id, "versioned", object);
            const auto& versionedObj = entryObj["versioned"];

            // 按照版本号进行排序，用于实现 “新版本数据继承上一版本数据” 的功能
            std::map<Version, const nlohmann::json*> sortedObjs;
            for (const auto& [versionStr, blockObj] : versionedObj.items())
            {
                if (blockObj.empty()) continue;
                CHECK_KEY(versionedObj, "versioned", versionStr, object);
                const Version version = Version::fromString(versionStr);
                sortedObjs[version] = &blockObj;
            }

            // 实际解析行为
            BlockData lastestBlock = entry.baseBlock;
            for (const auto& [version, blockObj] : sortedObjs)
            {
                BlockData block = lastestBlock;
                parseBlockData(*blockObj, version.toString().c_str(), block, false);
                lastestBlock = block;
                entry.versionedBlocks[version] = std::move(block);
            }
        }

        blockEntries[id] = entry;
    }
}

#undef CHECK_KEY
#undef THROW_KEY_UNCORRECT_TYPE_ERROR
#undef THROW_KEY_NOT_FOUND_ERROR

} // namespace

BlockEntryMap parseBlockEntries(std::string_view json)
{
    // 包装一层异常消息
    BlockEntryMap ret;
    try
    {
        parseBlockEntriesHelper(json, ret);
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(
            std::string("parseBlockEntries(): invalid json for parse 'Block Entry Map': ") + e.what()
        );
    }
    return ret;
}

BlockEntryMap parseBlockEntriesFromFile(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
        throw std::runtime_error("parseBlockEntriesFromFile(): can't open the file '" + filepath + "'");

    std::string json(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
    file.close();

    return parseBlockEntries(json);
}

BlockDataMap resolveBlockEntries(const BlockEntryMap& blockEntries)
{
    BlockDataMap ret;
    for (const auto& [id, entry] : blockEntries)
        ret[id] = &entry.baseBlock;
    return ret;
}

BlockDataMap resolveBlockEntries(const BlockEntryMap& blockEntries, Version targetVersion)
{
    BlockDataMap ret;
    for (const auto& [id, entry] : blockEntries)
    {
        // 如果加入版本比目标版本更新，说明在目标版本中此方块还未被加入，直接跳过。
        if (entry.minVersion > targetVersion)
            continue;

        // 目标是获取不超过 targetVersion 的最新版本的方块数据。
        Version lastestVersion(0, 0, 0);
        const BlockData* lastestBlock = &entry.baseBlock;
        for (const auto& [version, block] : entry.versionedBlocks)
        {
            if (version > targetVersion)
                continue;

            if (version > lastestVersion)
            {
                lastestVersion = version;
                lastestBlock   = &block;
            }
        }

        ret[id] = lastestBlock;
    }
    return ret;
}

BlockDataMap filterBlocks(const BlockDataMap& blocks, BlockAttributeFilterMode filterMode, BlockAttributes attributes)
{
    BlockDataMap ret;
    for (const auto& [id, block] : blocks)
    {
        switch (filterMode)
        {
            case BLOCK_ATTRI_FILTER_MODE_CONTAINS_ALL:
                if ((block->attributes & attributes) == attributes)        ret[id] = block; break;
            case BLOCK_ATTRI_FILTER_MODE_DISJOINT:
                if ((block->attributes & attributes) == 0)                 ret[id] = block; break;
            case BLOCK_ATTRI_FILTER_MODE_SUBSETOF:
                if ((block->attributes & attributes) == block->attributes) ret[id] = block; break;
            default:
                throw std::invalid_argument("filterBlocks(): invalid block attribute filter mode");
        }
    }
    return ret;
}
