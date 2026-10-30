#include "app_configuration.h"

#include <qsettings.h>

#include <app_defines.h>

ConfigurationData loadConfiguration()
{
    ConfigurationData config;
    QSettings         qsettings;

    config.colorScheme = qsettings.value(QStringLiteral("ColorScheme"), ColorSchemeManager::COLOR_SCHEME_AUTO)
        .value<ColorSchemeManager::ColorScheme>();
    config.language    = qsettings.value(QStringLiteral("Language"),    LanguageManager::LANGUAGE_AUTO)
        .value<LanguageManager::Language>();

    return config;
}

void saveConfiguration(const ConfigurationData& config)
{
    QSettings qsettings;

    qsettings.setValue(QStringLiteral("ColorScheme"), config.colorScheme);
    qsettings.setValue(QStringLiteral("Language"),    config.language);
}

ConfigurationData& getAppConfiguration()
{
    static ConfigurationData config;
    return config;
}

void setAppConfiguration(ConfigurationData config)
{
    getAppConfiguration() = std::move(config);
}
