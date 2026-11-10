#pragma once

#include <qbytearray.h>

#include <components/color_scheme_manager.h>
#include <components/language_manager.h>

#include <block.hpp>

class QTimer;

/** 程序的设置/配置单例类。 */
class AppConfiguration : public QObject
{
    Q_OBJECT

public:
    enum ConfigGroup : int
    {
        MainGroup = 0x0001,
        AllGroup  = 0xFFFF
    };

    static AppConfiguration& instance();

    void reload();
    void save(int groups = AllGroup) const;
    void saveLater(int groups = AllGroup, int delayMs = 3000) const;

    struct
    {
        ColorSchemeManager::ColorScheme colorScheme;
        LanguageManager::Language       language;
        QByteArray                      windowGeometry;
        bool                            sidebarExpanded;
        int                             sidebarCurrentIndex;
    } mainGroup;

    struct
    {
        BlockEntryMap blockEntries;
        BlockDataMap  baseBlocks;
    } dataCache;

private:
    AppConfiguration();
    ~AppConfiguration();

    static inline bool hasGroup(int groups, ConfigGroup group)
    { return (groups & group) != 0; }

    mutable int saveLaterGroups_ = 0;
    QTimer*     timer_;
};

#define appConfig AppConfiguration::instance()
