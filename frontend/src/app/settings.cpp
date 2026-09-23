#include "settings.h"

#include <qsettings.h>

Settings loadSettings()
{
    Settings  settings;
    QSettings qsettings;

    settings.colorScheme = qsettings.value("ColorScheme", ColorSchemeManager::COLOR_SCHEME_AUTO)
        .value<ColorSchemeManager::ColorScheme>();
    settings.language    = qsettings.value("Language",    LanguageManager::LANGUAGE_AUTO)
        .value<LanguageManager::Language>();

    return settings;
}

void saveSettings(const Settings& settings)
{
    QSettings qsettings;

    qsettings.setValue("ColorScheme", settings.colorScheme);
    qsettings.setValue("Language",    settings.language);
}
