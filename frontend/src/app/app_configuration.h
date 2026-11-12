#pragma once

#include <qbytearray.h>
#include <qlist.h>
#include <qstring.h>

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
        MainGroup            = 0x0001,
        BlockImageImageGroup = 0x0002,
        BlockImageVideoGroup = 0x0004,
        PixelArtImageGroup   = 0x0008,
        PixelArtVideoGroup   = 0x0010,
        AllGroup             = 0xFFFF
    };

    static AppConfiguration& instance();

    void reload();
    void save(int groups = AllGroup) const;
    void saveLater(int groups = AllGroup, int delayMs = 5000) const;

    struct
    {
        ColorSchemeManager::ColorScheme colorScheme;
        LanguageManager::Language       language;
        QByteArray                      windowGeometry;
        bool                            sidebarExpanded;
        int                             sidebarCurrentIndex;
    } mainGroup;

    struct CommonOptionConfig
    {
        // 共有部分
        int            outputFileMode;
        QString        customOutputDirectory;

        int            desiredSurface;

        int            fallbackSelectorMode;
        QString        selectedCustomBlock;

        QSize          maxFrameSize;

        int            checkedBlockAttributes;
        int            blockFilterMode;
        bool           filterPanelExpaned;
        QList<QString> selectedBlocks;

        // 视频部分
        int            maxFrameCount;
        int            numThreads;
    };

    struct MCPackOpionConfig
    {
        int     packType;
        QString packName;
        QString packDescription;
        QString packNamespace;
        Version packVersion;
        bool    autoFillPackName;
        bool    autoFillPackDescription;
        bool    assistFunctionEnabled;
    };

    CommonOptionConfig blockImageImageGroup, blockImageVideoGroup;
    struct PixelArtGeneratorConfig
    {
        CommonOptionConfig commonConfig;
        MCPackOpionConfig  mcpackConfig;
    } pixelArtImageGroup, pixelArtVideoGroup;

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
