#include "block_image_generator_widget.h"

#include <qlayout.h>

#include <components/tabbed_page_widget.h>
#include "block_image_generator_page_widget.h"

BlockImageGeneratorWidget::BlockImageGeneratorWidget(QWidget* parent)
    : AdaptiveWidget(parent)
{
    container_ = new TabbedPageWidget(this);
    imagePage_ = new BlockImageGeneratorPageWidget(BlockImageGeneratorPageWidget::ImageMode, this);
    videoPage_ = new BlockImageGeneratorPageWidget(BlockImageGeneratorPageWidget::VideoMode, this);

    container_->addTab(imagePage_, "", QIcon());
    container_->addTab(videoPage_, "", QIcon());
    container_->tabHeader(0)->setFixedHeight(36);
    container_->tabHeader(1)->setFixedHeight(36);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(container_);

    refreshText();
    refreshColor();
}

void BlockImageGeneratorWidget::refreshText()
{
    container_->setTabText(0, EASYTR("Image"));
    container_->setTabText(1, EASYTR("Video"));
}

void BlockImageGeneratorWidget::refreshColor()
{
    if (ColorSchemeManager::instance().loadedColorScheme() == ColorSchemeManager::COLOR_SCHEME_DARK)
    {
        container_->setTabIcon(0, QIcon(QStringLiteral(":/icons/app/image_light.svg")));
        container_->setTabIcon(1, QIcon(QStringLiteral(":/icons/app/video_light.svg")));
    }
    else
    {
        container_->setTabIcon(0, QIcon(QStringLiteral(":/icons/app/image_dark.svg")));
        container_->setTabIcon(1, QIcon(QStringLiteral(":/icons/app/video_dark.svg")));
    }
}
