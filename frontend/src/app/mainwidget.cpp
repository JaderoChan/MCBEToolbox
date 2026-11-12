#include "mainwidget.h"

#include <qapplication.h>
#include <qlayout.h>
#include <qstackedwidget.h>

#include <app_defines.h>
#include <block.hpp>
#include <components/sidebar_widget.h>
#include <nbt_editor/nbt_editor_widget.h>
#include <utils/directory_scope.h>
#include "app_configuration.h"
#include "block_image_generator_widget.h"
#include "pixel_art_generator_widget.h"
#include "settings_widget.h"

MainWidget::MainWidget(QWidget* parent)
    : AdaptiveWidget(parent)
{
    // Sidebar widget
    sidebarWidget_ = new SidebarWidget(this);
    sidebarWidget_->setAutoFillBackground(true);
    sidebarWidget_->setExpandedWidth(280);
    sidebarWidget_->setCollapsedWidth(48);
    sidebarWidget_->setIconSize(28);

    sidebarWidget_->addEntry(QIcon(), QString());
    sidebarWidget_->addEntry(QIcon(), QString());
    sidebarWidget_->addEntry(QIcon(), QString());
    sidebarWidget_->addStretch();
    sidebarWidget_->addEntry(QIcon(), QString());
    sidebarWidget_->setCurrentIndex(0);

    // Stacked widget
    stackedWidget_ = new QStackedWidget(this);
    stackedWidget_->setContentsMargins(0, 0, 0, 0);
    stackedWidget_->addWidget(new PixelArtGeneratorWidget(this));
    stackedWidget_->addWidget(new BlockImageGeneratorWidget(this));
    stackedWidget_->addWidget(new NbtEditorWidget(this));
    stackedWidget_->addWidget(new SettingsWidget(this));
    stackedWidget_->setCurrentIndex(0);

    // Main layout
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(sidebarWidget_);
    mainLayout->addWidget(stackedWidget_);

    // 加载程序配置。
    const auto& config = appConfig.mainGroup;
    if (!restoreGeometry(config.windowGeometry))
        resize(1000, 700);
    config.sidebarExpanded ? sidebarWidget_->expand() : sidebarWidget_->fold();
    sidebarWidget_->setCurrentIndex(config.sidebarCurrentIndex);

    connect(sidebarWidget_, &SidebarWidget::currentIndexChanged, stackedWidget_, &QStackedWidget::setCurrentIndex);
    connect(
        &LanguageManager::instance(), &LanguageManager::languageChanged,
        &LanguageManager::instance(), [](LanguageManager::Language language)
        {
            appConfig.mainGroup.language = language;
            appConfig.saveLater(AppConfiguration::MainGroup);
        }
    );
    connect(
        &ColorSchemeManager::instance(), &ColorSchemeManager::colorSchemeChanged,
        &ColorSchemeManager::instance(), [](ColorSchemeManager::ColorScheme scheme)
        {
            appConfig.mainGroup.colorScheme = scheme;
            appConfig.saveLater(AppConfiguration::MainGroup);
        }
    );
    connect(sidebarWidget_, &SidebarWidget::expandedChanged, sidebarWidget_, [](bool expanded)
    {
        appConfig.mainGroup.sidebarExpanded = expanded;
        appConfig.saveLater(AppConfiguration::MainGroup);
    });
    connect(sidebarWidget_, &SidebarWidget::currentIndexChanged, sidebarWidget_, [](int index)
    {
        appConfig.mainGroup.sidebarCurrentIndex = index;
        appConfig.saveLater(AppConfiguration::MainGroup);
    });

    refreshText();
    refreshColor();
}

void MainWidget::refreshText()
{
    setWindowTitle(EASYTR(APP_TITLE));
    sidebarWidget_->setEntryText(0, EASYTR("Pixel-Art Generator"));
    sidebarWidget_->setEntryText(1, EASYTR("Block Image/Video Generator"));
    sidebarWidget_->setEntryText(2, EASYTR("NBT Editor"));
    sidebarWidget_->setEntryText(3, EASYTR("Settings"));
    sidebarWidget_->setEntryToolTip(0, EASYTR("Pixel-Art Generator"));
    sidebarWidget_->setEntryToolTip(1, EASYTR("Block Image/Video Generator"));
    sidebarWidget_->setEntryToolTip(2, EASYTR("NBT Editor"));
    sidebarWidget_->setEntryToolTip(3, EASYTR("Settings"));
}

void MainWidget::refreshColor()
{
    if (ColorSchemeManager::instance().loadedColorScheme() == ColorSchemeManager::COLOR_SCHEME_DARK)
    {
        auto palette     = qApp->palette();
        const auto color = palette.brush(QPalette::Window).color().lighter(128);
        palette.setColor(QPalette::ColorRole::Window, color);
        sidebarWidget_->setPalette(palette);

        sidebarWidget_->setEntryIcon(0, QIcon(QStringLiteral(":/icons/app/pixel_art_generator_light.svg")));
        sidebarWidget_->setEntryIcon(1, QIcon(QStringLiteral(":/icons/app/block_image_generator_light.svg")));
        sidebarWidget_->setEntryIcon(2, QIcon(QStringLiteral(":/icons/app/nbt_editor_light.svg")));
        sidebarWidget_->setEntryIcon(3, QIcon(QStringLiteral(":/icons/app/settings_light.svg")));
    }
    else
    {
        auto palette     = qApp->palette();
        const auto color = palette.brush(QPalette::Window).color().darker(128);
        palette.setColor(QPalette::ColorRole::Window, color);
        sidebarWidget_->setPalette(palette);

        sidebarWidget_->setEntryIcon(0, QIcon(QStringLiteral(":/icons/app/pixel_art_generator_dark.svg")));
        sidebarWidget_->setEntryIcon(1, QIcon(QStringLiteral(":/icons/app/block_image_generator_dark.svg")));
        sidebarWidget_->setEntryIcon(2, QIcon(QStringLiteral(":/icons/app/nbt_editor_dark.svg")));
        sidebarWidget_->setEntryIcon(3, QIcon(QStringLiteral(":/icons/app/settings_dark.svg")));
    }
}

void MainWidget::closeEvent(QCloseEvent* e)
{
    appConfig.mainGroup.windowGeometry = saveGeometry();
    appConfig.save(AppConfiguration::MainGroup);
    AdaptiveWidget::closeEvent(e);
}
