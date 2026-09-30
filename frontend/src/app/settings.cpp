#include "settings.h"

#include <qsettings.h>

Settings loadSettings()
{
    Settings  settings;
    QSettings qsettings;

    settings.colorScheme = qsettings.value(QStringLiteral("ColorScheme"), ColorSchemeManager::COLOR_SCHEME_AUTO)
        .value<ColorSchemeManager::ColorScheme>();
    settings.language    = qsettings.value(QStringLiteral("Language"),    LanguageManager::LANGUAGE_AUTO)
        .value<LanguageManager::Language>();

    return settings;
}

void saveSettings(const Settings& settings)
{
    QSettings qsettings;

    qsettings.setValue(QStringLiteral("ColorScheme"), settings.colorScheme);
    qsettings.setValue(QStringLiteral("Language"),    settings.language);
}
