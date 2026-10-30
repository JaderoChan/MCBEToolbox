#include "app_configuration.h"

#include <stdio.h>

#include <qsettings.h>

#include <app_defines.h>
#include <utils/directory_scope.h>

ConfigurationData loadConfiguration()
{
    ConfigurationData config;
    QSettings         qsettings;

    config.colorScheme = qsettings.value(QStringLiteral("ColorScheme"), ColorSchemeManager::COLOR_SCHEME_AUTO)
        .value<ColorSchemeManager::ColorScheme>();
    config.language    = qsettings.value(QStringLiteral("Language"),    LanguageManager::LANGUAGE_AUTO)
        .value<LanguageManager::Language>();

    try
    {
        DirectoryScope dirScope(APP_RESOURCES_DIRPATH);
        config.blockEntries = parseBlockEntriesFromFile(APP_BLOCK_ENTRIES_FILEPATH);
    }
    catch (std::exception& e)
    {
        fprintf(
            stderr,
            "loadConfiguration() Failed to parse block entries from file '%s'\n",
            APP_BLOCK_ENTRIES_FILEPATH
        );
    }

    return config;
}

void saveConfiguration(const ConfigurationData& config)
{
    QSettings qsettings;

    qsettings.setValue(QStringLiteral("ColorScheme"), config.colorScheme);
    qsettings.setValue(QStringLiteral("Language"),    config.language);
}

static ConfigurationData AppConfig;

const ConfigurationData& getAppConfiguration()
{
    return AppConfig;
}

ConfigurationData& getAppConfigurationRef()
{
    return AppConfig;
}

void setAppConfiguration(ConfigurationData config)
{
    AppConfig = std::move(config);
}
