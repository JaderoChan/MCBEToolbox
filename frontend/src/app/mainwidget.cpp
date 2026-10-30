#include "mainwidget.h"

#include <qlayout.h>
#include <qstackedwidget.h>

#include <block.hpp>
#include <app_defines.h>
#include <components/sidebar_widget.h>
#include <nbt_editor/nbt_editor_widget.h>
#include <utils/directory_scope.h>
#include "app_configuration.h"
#include "block_list_filter_widget.h"

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
    DirectoryScope dirScope(APP_RESOURCES_DIRPATH);
    static const BlockEntryMap& entries = parseBlockEntriesFromFile(APP_BLOCK_ENTRIES_FILEPATH);
    auto* wgt = new BlockListFilterWidget(appConfiguration().blockEntries, this);

    stackedWidget_ = new QStackedWidget(this);
    stackedWidget_->setContentsMargins(0, 0, 0, 0);
    stackedWidget_->addWidget(wgt); // TODO
    stackedWidget_->addWidget(new QWidget(this)); // TODO
    stackedWidget_->addWidget(new NbtEditorWidget(this));
    stackedWidget_->addWidget(new QWidget(this)); // TODO
    stackedWidget_->setCurrentIndex(0);

    // Main layout
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(sidebarWidget_);
    mainLayout->addWidget(stackedWidget_);

    connect(sidebarWidget_, &SidebarWidget::currentIndexChanged, stackedWidget_, &QStackedWidget::setCurrentIndex);

    resize(1080, 720);
    updateText();
    updateColor();
}

void MainWidget::updateText(LanguageManager::Language loaded)
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

void MainWidget::updateColor(ColorSchemeManager::ColorScheme loaded)
{
    if (loaded == ColorSchemeManager::COLOR_SCHEME_DARK)
    {
        auto palette      = qApp->palette();
        const auto& color = palette.brush(QPalette::Window).color().lighter(135);
        palette.setColor(QPalette::ColorRole::Window, color);
        sidebarWidget_->setPalette(palette);

        sidebarWidget_->setEntryIcon(0, QIcon(QStringLiteral(":/icons/app/pixel_art_generator_light.svg")));
        sidebarWidget_->setEntryIcon(1, QIcon(QStringLiteral(":/icons/app/block_image_generator_light.svg")));
        sidebarWidget_->setEntryIcon(2, QIcon(QStringLiteral(":/icons/app/nbt_editor_light.svg")));
        sidebarWidget_->setEntryIcon(3, QIcon(QStringLiteral(":/icons/app/settings_light.svg")));
    }
    else
    {
        auto palette      = qApp->palette();
        const auto& color = palette.brush(QPalette::Window).color().darker(150);
        palette.setColor(QPalette::ColorRole::Window, color);
        sidebarWidget_->setPalette(palette);

        sidebarWidget_->setEntryIcon(0, QIcon(QStringLiteral(":/icons/app/pixel_art_generator_dark.svg")));
        sidebarWidget_->setEntryIcon(1, QIcon(QStringLiteral(":/icons/app/block_image_generator_dark.svg")));
        sidebarWidget_->setEntryIcon(2, QIcon(QStringLiteral(":/icons/app/nbt_editor_dark.svg")));
        sidebarWidget_->setEntryIcon(3, QIcon(QStringLiteral(":/icons/app/settings_dark.svg")));
    }
}
