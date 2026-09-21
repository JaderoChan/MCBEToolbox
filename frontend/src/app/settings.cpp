#include "settings.h"

#include <qsettings.h>

Settings loadSettings()
{
    Settings settings;
    QSettings qsettings;

    settings.language = qsettings.value("Language", getCurrentSystemLang()).value<Language>();

    return settings;
}

void saveSettings(const Settings& settings)
{
    QSettings qsettings;

    qsettings.setValue("Language", settings.language);
}
