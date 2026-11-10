#include "app_configuration.h"

#include <qsettings.h>
#include <qtimer.h>

#include <nlohmann/json.hpp>

#include <app_defines.h>
#include <utils/debug_output.h>
#include <utils/directory_scope.h>

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

    try
    {
        DirectoryScope dirScope(APP_RESOURCES_DIRPATH);
        dataCache.blockEntries = parseBlockEntriesFromFile(APP_BLOCK_ENTRIES_FILEPATH);
    }
    catch (std::exception& e)
    {
        debugOut
            (qWarning(),
            "loadConfiguration() Failed to parse block entries from file '%1'",
            APP_BLOCK_ENTRIES_FILEPATH
        );
    }
    dataCache.baseBlocks = resolveBlockEntries(dataCache.blockEntries);
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
}

void AppConfiguration::saveLater(int groups, int delayMs) const
{
    saveLaterGroups_ |= groups;
    timer_->start(delayMs);
}
