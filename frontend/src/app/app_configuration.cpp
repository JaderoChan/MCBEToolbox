#include "app_configuration.h"

#include <filesystem>
#include <fstream>

#include <qdir.h>
#include <qsettings.h>
#include <qtimer.h>

#include <nlohmann/json.hpp>

#include <app_defines.h>
#include <app/fallback_block_selector.h>
#include <app/mcpack_generator_option_widget.h>
#include <components/output_file_selector.h>
#include <utils/debug_output.h>
#include <utils/directory_scope.h>

namespace
{

#define BLOCK_IMAGE_IMAGE_CONFIG_FILEPATH "block_image_image.config"
#define BLOCK_IMAGE_VIDEO_CONFIG_FILEPATH "block_image_video.config"
#define PIXEL_ART_IMAGE_CONFIG_FILEPATH   "pixel_art_image.config"
#define PIXEL_ART_VIDEO_CONFIG_FILEPATH   "pixel_art_video.config"

constexpr int              DEFAULT_OUTPUT_FILE_MODE         = OutputFileSelector::ToSourceDirectoryMode;
constexpr int              DEFAULT_FALLBACK_SELECTOR_MODE   = FallbackBlockSelector::NoneBlockMode;
constexpr int              DEFAULT_CHECKED_BLOCK_ATTRIBUTES = BLOCK_ATTRI_IS_TRANSPARENT | BLOCK_ATTRI_IS_UNSTABLE | BLOCK_ATTRI_IS_INCOMPLETE | BLOCK_ATTRI_IS_UNSTABLE | BLOCK_ATTRI_HAS_PATTERN;
constexpr int              DEFAULT_BLOCK_FILTER_MODE        = BLOCK_ATTRI_FILTER_MODE_CONTAINS_ANY;
constexpr int              DEFAULT_MAX_FRAME_COUNT          = -1;
constexpr int              DEFAULT_NUM_THREADS              = 8;
constexpr int              DEFAULT_PACK_TYPE                = MCPackGeneratorOptionWidget::StructurePackType;
constexpr const char*      DEFAULT_SELECTED_CUSTOM_BLOCK    = "minecraft:glass_group:glass";
constexpr const char*      DEFAULT_PACK_NAMESPACE           = "mcbetoolbox";
constexpr Version          DEFAULT_PACK_VERSION             = Version(1, 0, 0);
constexpr SurfaceDirection DEFAULT_DESIRED_SURFACE          = SURFACE_DIRECTION_NORTH;

BlockEntryMap loadBlockEntries()
{
    try
    {
        DirectoryScope dirScope(APP_RESOURCES_DIRPATH);
        return parseBlockEntriesFromFile(APP_BLOCK_ENTRIES_FILEPATH);
    }
    catch (std::exception& e)
    {
        debugOut(
            qWarning(),
            "loadBlockEntries() Failed to parse block entries from file '%1': %2",
            APP_BLOCK_ENTRIES_FILEPATH,
            e.what()
        );
        return BlockEntryMap();
    }
}

QList<QString> getDefaultSelectedBlocks()
{
    const auto blockEntries = loadBlockEntries();
    QList<QString> blockIds;
    for (const auto& [id, entry] : blockEntries)
    {
        if (!HAS_ANY_BLOCK_ATTRIS(entry.baseBlock.attributes, DEFAULT_CHECKED_BLOCK_ATTRIBUTES))
            blockIds.append(QString::fromStdString(std::string(id)));
    }
    return blockIds;
}

AppConfiguration::CommonOptionConfig getDefaultCommonOptionConfig(const QSize& maxFrameSize)
{
    return AppConfiguration::CommonOptionConfig
    {
        DEFAULT_OUTPUT_FILE_MODE,
        QString(),
        DEFAULT_DESIRED_SURFACE,
        DEFAULT_FALLBACK_SELECTOR_MODE,
        DEFAULT_SELECTED_CUSTOM_BLOCK,
        maxFrameSize,
        DEFAULT_CHECKED_BLOCK_ATTRIBUTES,
        DEFAULT_BLOCK_FILTER_MODE,
        false,
        getDefaultSelectedBlocks(),
        DEFAULT_MAX_FRAME_COUNT,
        DEFAULT_NUM_THREADS
    };
}

AppConfiguration::MCPackOptionConfig getDefaultMCPackOptionConfig()
{
    return AppConfiguration::MCPackOptionConfig
    {
        DEFAULT_PACK_TYPE,
        QString(),
        QString(),
        DEFAULT_PACK_NAMESPACE,
        DEFAULT_PACK_VERSION,
        true,
        true,
        true
    };
}

AppConfiguration::CommonOptionConfig parseCommonOptionConfig(const nlohmann::json& json, const QSize& maxFrameSize)
{
    if (json.is_discarded() || !json.is_object())
        return getDefaultCommonOptionConfig(maxFrameSize);

    AppConfiguration::CommonOptionConfig config;

    config.outputFileMode        = json.value("output_file_mode",       DEFAULT_OUTPUT_FILE_MODE);
    config.desiredSurface        = json.value("desired_direction",      DEFAULT_DESIRED_SURFACE);
    config.fallbackSelectorMode  = json.value("fallback_selector_mode", DEFAULT_FALLBACK_SELECTOR_MODE);

    config.customOutputDirectory = QString::fromStdString(json.value("custom_output_directory", std::string()));
    config.selectedCustomBlock   = QString::fromStdString(json.value("selected_custom_block",   DEFAULT_SELECTED_CUSTOM_BLOCK));

    config.maxFrameSize.setWidth( json.value("max_frame_size_width",  maxFrameSize.width()));
    config.maxFrameSize.setHeight(json.value("max_frame_size_height", maxFrameSize.height()));

    config.checkedBlockAttributes = json.value("checked_block_attributes", DEFAULT_CHECKED_BLOCK_ATTRIBUTES);
    config.blockFilterMode        = json.value("block_filter_mode",        DEFAULT_BLOCK_FILTER_MODE);
    config.filterPanelExpanded    = json.value("filter_panel_expanded",    false);

    if (json.contains("selected_blocks") && json["selected_blocks"].is_array())
    {
        config.selectedBlocks.clear();
        for (const auto& item : json["selected_blocks"])
            if (item.is_string())
                config.selectedBlocks.append(QString::fromStdString(item.get<std::string>()));
    }
    else
    {
        config.selectedBlocks = getDefaultSelectedBlocks();
    }

    config.maxFrameCount          = json.value("max_frame_count",          DEFAULT_MAX_FRAME_COUNT);
    config.numThreads             = json.value("num_threads",              DEFAULT_NUM_THREADS);

    // 处理非法值。
    if (config.outputFileMode != OutputFileSelector::ToSourceDirectoryMode &&
        config.outputFileMode != OutputFileSelector::ToCustomDirectoryMode &&
        config.outputFileMode != OutputFileSelector::ToCustomFileMode)
        config.outputFileMode = DEFAULT_OUTPUT_FILE_MODE;
    if (config.desiredSurface < SURFACE_DIRECTION_UP || config.desiredSurface > SURFACE_DIRECTION_WEST)
        config.desiredSurface = DEFAULT_DESIRED_SURFACE;
    if (config.fallbackSelectorMode != FallbackBlockSelector::NoneBlockMode &&
        config.fallbackSelectorMode != FallbackBlockSelector::AirBlockMode  &&
        config.fallbackSelectorMode != FallbackBlockSelector::CustomBlockMode)
        config.fallbackSelectorMode = DEFAULT_FALLBACK_SELECTOR_MODE;
    if (config.blockFilterMode != BLOCK_ATTRI_FILTER_MODE_CONTAINS_ALL &&
        config.blockFilterMode != BLOCK_ATTRI_FILTER_MODE_CONTAINS_ANY)
        config.blockFilterMode = DEFAULT_BLOCK_FILTER_MODE;
    if (config.maxFrameCount < -1)
        config.maxFrameCount = DEFAULT_MAX_FRAME_COUNT;
    if (config.numThreads < 0)
        config.numThreads = DEFAULT_NUM_THREADS;

    return config;
}

AppConfiguration::MCPackOptionConfig parseMCPackOptionConfig(const nlohmann::json& json)
{
    if (json.is_discarded() || !json.is_object())
        return getDefaultMCPackOptionConfig();

    AppConfiguration::MCPackOptionConfig config;

    config.packType                = json.value("pack_type", DEFAULT_PACK_TYPE);
    config.packName                = QString::fromStdString(json.value("pack_name",        std::string()));
    config.packDescription         = QString::fromStdString(json.value("pack_description", std::string()));
    config.packNamespace           = QString::fromStdString(json.value("pack_namespace",   DEFAULT_PACK_NAMESPACE));
    config.autoFillPackName        = json.value("auto_fill_pack_name",        true);
    config.autoFillPackDescription = json.value("auto_fill_pack_description", true);
    config.assistFunctionEnabled   = json.value("assist_function_enabled",    true);

    if (!json.contains("pack_version") || !json["pack_version"].is_string())
    {
        config.packVersion = DEFAULT_PACK_VERSION;
    }
    else
    {
        const std::string packVersionStr = json["pack_version"];
        try { config.packVersion = Version::fromString(packVersionStr); }
        catch (...) { config.packVersion = DEFAULT_PACK_VERSION; }
    }

    // 处理非法值。
    if (config.packType != MCPackGeneratorOptionWidget::StructurePackType &&
        config.packType != MCPackGeneratorOptionWidget::FunctionPackType)
        config.packType = DEFAULT_PACK_TYPE;

    return config;
}

nlohmann::json serializeCommonOptionConfig(const AppConfiguration::CommonOptionConfig& config)
{
    nlohmann::json json = nlohmann::json::object();

    json["output_file_mode"]         = config.outputFileMode;
    json["desired_direction"]        = config.desiredSurface;
    json["fallback_selector_mode"]   = config.fallbackSelectorMode;
    json["custom_output_directory"]  = config.customOutputDirectory.toStdString();
    json["selected_custom_block"]    = config.selectedCustomBlock.toStdString();
    json["max_frame_size_width"]     = config.maxFrameSize.width();
    json["max_frame_size_height"]    = config.maxFrameSize.height();
    json["checked_block_attributes"] = config.checkedBlockAttributes;
    json["block_filter_mode"]        = config.blockFilterMode;
    json["filter_panel_expanded"]    = config.filterPanelExpanded;
    json["max_frame_count"]          = config.maxFrameCount;
    json["num_threads"]              = config.numThreads;

    nlohmann::json selectedBlocksJson = nlohmann::json::array();
    for (const auto& id : config.selectedBlocks)
        selectedBlocksJson.push_back(id.toStdString());
    json["selected_blocks"] = selectedBlocksJson;

    return json;
}

nlohmann::json serializeMCPackOptionConfig(const AppConfiguration::MCPackOptionConfig& config)
{
    nlohmann::json json = nlohmann::json::object();

    json["pack_type"]                  = config.packType;
    json["pack_name"]                  = config.packName.toStdString();
    json["pack_description"]           = config.packDescription.toStdString();
    json["pack_namespace"]             = config.packNamespace.toStdString();
    json["pack_version"]               = config.packVersion.toString();
    json["auto_fill_pack_name"]        = config.autoFillPackName;
    json["auto_fill_pack_description"] = config.autoFillPackDescription;
    json["assist_function_enabled"]    = config.assistFunctionEnabled;

    return json;
}

bool dumpJsonToFile(const nlohmann::json& json, const QString& filePath)
{
#ifdef Q_OS_WIN
    std::filesystem::path path(filePath.toStdWString());
#else
    std::filesystem::path path(filePath.toStdString());
#endif // Q_OS_WIJ

    std::ofstream ofs(path);
    if (!ofs.is_open())
    {
        debugOut(qWarning(), "dumpJsonToFile(): Failed save the json to '%1'", filePath);
        return false;
    }

    ofs << json.dump();
    return true;
}

nlohmann::json loadJsonFromFile(const QString& filePath)
{
#ifdef Q_OS_WIN
    std::filesystem::path path(filePath.toStdWString());
#else
    std::filesystem::path path(filePath.toStdString());
#endif // Q_OS_WIN

    std::ifstream ifs(path);
    if (!ifs.is_open())
        return nlohmann::json(nlohmann::json::value_t::discarded);

    return nlohmann::json::parse(ifs, nullptr, false, true);
}

} // namespace

AppConfiguration::AppConfiguration()
    : QObject(nullptr)
{
    timer_ = new QTimer(this);
    timer_->setSingleShot(true);
    connect(timer_, &QTimer::timeout, this, [this]()
    {
        save(saveLaterGroups_);
        saveLaterGroups_ = 0;
    });

    reload();
}

AppConfiguration::~AppConfiguration()
{
    save();
}

AppConfiguration& AppConfiguration::instance()
{
    static AppConfiguration instance;
    return instance;
}

void AppConfiguration::reload()
{
    QSettings qsettings;

    mainGroup.colorScheme         = qsettings
        .value(QStringLiteral("ColorScheme"), ColorSchemeManager::COLOR_SCHEME_AUTO)
        .value<ColorSchemeManager::ColorScheme>();
    mainGroup.language            = qsettings
        .value(QStringLiteral("Language"),    LanguageManager::LANGUAGE_AUTO)
        .value<LanguageManager::Language>();
    mainGroup.windowGeometry      = qsettings.value(QStringLiteral("WindowGeometry"),      QByteArray()).toByteArray();
    mainGroup.sidebarExpanded     = qsettings.value(QStringLiteral("SidebarExpanded"),     true        ).toBool();
    mainGroup.sidebarCurrentIndex = qsettings.value(QStringLiteral("SidebarCurrentIndex"), 0           ).toInt();

    {
        QDir().mkpath(APP_CONFIG_DIRPATH);
        DirectoryScope dirScope(APP_CONFIG_DIRPATH);
        {
            constexpr QSize MAX_FRAME_SIZE = QSize(1024, 1024);
            const nlohmann::json json = loadJsonFromFile(BLOCK_IMAGE_IMAGE_CONFIG_FILEPATH);
            blockImageImageGroup = json.is_discarded() ? getDefaultCommonOptionConfig(MAX_FRAME_SIZE) : parseCommonOptionConfig(json, MAX_FRAME_SIZE);
        }
        {
            constexpr QSize MAX_FRAME_SIZE = QSize(256, 128);
            const nlohmann::json json = loadJsonFromFile(BLOCK_IMAGE_VIDEO_CONFIG_FILEPATH);
            blockImageVideoGroup = json.is_discarded() ? getDefaultCommonOptionConfig(MAX_FRAME_SIZE) : parseCommonOptionConfig(json, MAX_FRAME_SIZE);
        }
        {
            constexpr QSize MAX_FRAME_SIZE = QSize(540, 360);
            const nlohmann::json json = loadJsonFromFile(PIXEL_ART_IMAGE_CONFIG_FILEPATH);
            pixelArtImageGroup.commonConfig =
                json.contains("common_config")
                ? parseCommonOptionConfig(json["common_config"], MAX_FRAME_SIZE)
                : getDefaultCommonOptionConfig(MAX_FRAME_SIZE);
            pixelArtImageGroup.mcpackConfig =
                json.contains("mcpack_config")
                ? parseMCPackOptionConfig(json["mcpack_config"])
                : getDefaultMCPackOptionConfig();
        }
        {
            constexpr QSize MAX_FRAME_SIZE = QSize(540, 360);
            const nlohmann::json json = loadJsonFromFile(PIXEL_ART_VIDEO_CONFIG_FILEPATH);
            pixelArtVideoGroup.commonConfig =
                json.contains("common_config")
                ? parseCommonOptionConfig(json["common_config"], MAX_FRAME_SIZE)
                : getDefaultCommonOptionConfig(MAX_FRAME_SIZE);
            pixelArtVideoGroup.mcpackConfig =
                json.contains("mcpack_config")
                ? parseMCPackOptionConfig(json["mcpack_config"])
                : getDefaultMCPackOptionConfig();
        }
    }

    dataCache.blockEntries = loadBlockEntries();
    dataCache.baseBlocks   = resolveBlockEntries(dataCache.blockEntries);
}

void AppConfiguration::save(int groups) const
{
    if (hasGroup(groups, MainGroup))
    {
        QSettings qsettings;
        qsettings.setValue(QStringLiteral("ColorScheme"),          mainGroup.colorScheme);
        qsettings.setValue(QStringLiteral("Language"),             mainGroup.language);
        qsettings.setValue(QStringLiteral("WindowGeometry"),       mainGroup.windowGeometry);
        qsettings.setValue(QStringLiteral("SidebarExpanded"),      mainGroup.sidebarExpanded);
        qsettings.setValue(QStringLiteral("SidebarCurrentIndex"),  mainGroup.sidebarCurrentIndex);
    }

    if (groups & (BlockImageImageGroup | BlockImageVideoGroup | PixelArtImageGroup | PixelArtVideoGroup))
        QDir().mkpath(APP_CONFIG_DIRPATH);

    if(hasGroup(groups, BlockImageImageGroup))
    {
        DirectoryScope dirScope(APP_CONFIG_DIRPATH);
        const auto json = serializeCommonOptionConfig(blockImageImageGroup);
        dumpJsonToFile(json, BLOCK_IMAGE_IMAGE_CONFIG_FILEPATH);
    }

    if(hasGroup(groups, BlockImageVideoGroup))
    {
        DirectoryScope dirScope(APP_CONFIG_DIRPATH);
        const auto json = serializeCommonOptionConfig(blockImageVideoGroup);
        dumpJsonToFile(json, BLOCK_IMAGE_VIDEO_CONFIG_FILEPATH);
    }

    if (hasGroup(groups, PixelArtImageGroup))
    {
        DirectoryScope dirScope(APP_CONFIG_DIRPATH);
        nlohmann::json json = nlohmann::json::object();
        json["common_config"] = serializeCommonOptionConfig(pixelArtImageGroup.commonConfig);
        json["mcpack_config"] = serializeMCPackOptionConfig(pixelArtImageGroup.mcpackConfig);
        dumpJsonToFile(json, PIXEL_ART_IMAGE_CONFIG_FILEPATH);
    }

    if (hasGroup(groups, PixelArtVideoGroup))
    {
        DirectoryScope dirScope(APP_CONFIG_DIRPATH);
        nlohmann::json json = nlohmann::json::object();
        json["common_config"] = serializeCommonOptionConfig(pixelArtVideoGroup.commonConfig);
        json["mcpack_config"] = serializeMCPackOptionConfig(pixelArtVideoGroup.mcpackConfig);
        dumpJsonToFile(json, PIXEL_ART_VIDEO_CONFIG_FILEPATH);
    }
}

void AppConfiguration::saveLater(int groups, int delayMs) const
{
    saveLaterGroups_ |= groups;
    timer_->start(delayMs);
}
