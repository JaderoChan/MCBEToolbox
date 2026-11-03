#include "fallback_block_selector.h"

#include <qbuttongroup.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qtimer.h>
#include <qtoolbutton.h>

#include <app_defines.h>
#include <utils/directory_scope.h>
#include "app_configuration.h"
#include "block_list_widget.h"

class CustomBlockSelectorDialog : public AdaptiveDialog
{
    Q_OBJECT

public:
    explicit CustomBlockSelectorDialog(BlockDataMap blocks, QWidget* parent = nullptr)
        : AdaptiveDialog(parent)
    {
        widget_        = new BlockListWidget(this);
        confirmButton_ = new QPushButton(this);
        cancelButton_  = new QPushButton(this);

        widget_->setBlocks(std::move(blocks));
        widget_->setMultiSelectEnabled(false);

        confirmButton_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        cancelButton_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        connect(confirmButton_, &QPushButton::clicked, this, &CustomBlockSelectorDialog::accept);
        connect(cancelButton_,  &QPushButton::clicked, this, &CustomBlockSelectorDialog::reject);

        auto* buttonWidget = new QWidget(this);
        auto* buttonLayout = new QHBoxLayout(buttonWidget);
        buttonLayout->setContentsMargins(3, 0, 3, 0);
        buttonLayout->setSpacing(6);
        buttonLayout->addWidget(confirmButton_);
        buttonLayout->addWidget(cancelButton_);

        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(widget_);
        mainLayout->addWidget(buttonWidget);

        refreshText();
    }

    BlockDataPair selected() const
    {
        if (widget_->selecteds().empty()) return {std::string_view(), nullptr};
        else                              return *widget_->selecteds().begin();
    }

    void setSelected(const QString& blockId)
    {
        widget_->select(blockId);
        QTimer::singleShot(0, this, [this, blockId]() { widget_->scrollTo(blockId); });
    }

    QSize sizeHint() const override
    {
        return QSize(640, 640);
    }

protected:
    void refreshText() override
    {
        setWindowTitle(EASYTR("Select Custom Fallback Block"));
        confirmButton_->setText(EASYTR("Confirm"));
        cancelButton_->setText(EASYTR("Cancel"));
    }

private:
    friend class FallbackBlockSelector;

    BlockListWidget* widget_        = nullptr;
    QPushButton*     confirmButton_ = nullptr;
    QPushButton*     cancelButton_  = nullptr;
};

FallbackBlockSelector::FallbackBlockSelector(QWidget* parent)
    : AdaptiveWidget(parent)
{
    buttonGroup_               = new QButtonGroup(this);
    noneBlockButton_           = new QToolButton(this);
    airBlockButton_            = new QToolButton(this);
    customBlockButton_         = new QToolButton(this);
    customBlockSelectorButton_ = new QToolButton(this);

    buttonGroup_->setExclusive(true);
    for (auto& button : {noneBlockButton_, airBlockButton_, customBlockButton_})
    {
        button->setCheckable(true);
        buttonGroup_->addButton(button);
    }
    customBlockButton_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    connect(noneBlockButton_,   &QToolButton::clicked, this, [this]() { setMode(NoneBlockMode);   });
    connect(airBlockButton_,    &QToolButton::clicked, this, [this]() { setMode(AirBlockMode);    });
    connect(customBlockButton_, &QToolButton::clicked, this, [this]() { setMode(CustomBlockMode); });
    connect(customBlockSelectorButton_, &QToolButton::clicked, this, [this]()
    {
        CustomBlockSelectorDialog dialog(customBlocks_);
        dialog.setSelected(QString::fromStdString(std::string(selectedCustomBlock_.first)));
        if (dialog.exec() == QDialog::Accepted)
        {
            selectedCustomBlock_ = std::move(dialog.selected());
            refreshCustomButtonTextIcon();
        }
    });

    auto* customButtonWidget = new QWidget(this);
    auto* customButtonLayout = new QHBoxLayout(customButtonWidget);
    customButtonLayout->setContentsMargins(0, 0, 0, 0);
    customButtonLayout->setSpacing(1);
    customButtonLayout->addWidget(customBlockButton_);
    customButtonLayout->addWidget(customBlockSelectorButton_);

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(noneBlockButton_);
    mainLayout->addWidget(airBlockButton_);
    mainLayout->addWidget(customButtonWidget);
    mainLayout->addStretch();

    refreshText();
    refreshButtonState();
}

void FallbackBlockSelector::setMode(Mode mode)
{
    if (mode_ != mode)
    {
        mode_ = mode;
        refreshButtonState();
    }
}

FallbackBlockSelector::Mode FallbackBlockSelector::mode() const
{
    return mode_;
}

void FallbackBlockSelector::setModeButtonVisible(Mode mode, bool visible)
{
    switch (mode)
    {
        case NoneBlockMode:   noneBlockButton_->setVisible(visible);   break;
        case AirBlockMode:    airBlockButton_->setVisible(visible);    break;
        case CustomBlockMode: customBlockButton_->setVisible(visible); break;
        default: break;
    }
}

bool FallbackBlockSelector::modeButtonVisible(Mode mode) const
{
    switch (mode)
    {
        case NoneBlockMode:   return noneBlockButton_->isVisible();
        case AirBlockMode:    return airBlockButton_->isVisible();
        case CustomBlockMode: return customBlockButton_->isVisible();
        default:              return false;
    }
}

void FallbackBlockSelector::setCustomBlocks(BlockDataMap blocks)
{
    customBlocks_ = std::move(blocks);
    refreshCustomButtonTextIcon();
}

const BlockDataMap& FallbackBlockSelector::customBlocks() const
{
    return customBlocks_;
}

void FallbackBlockSelector::setSelectedCustomBlock(const QString& blockId)
{
    auto it = customBlocks_.find(blockId.toStdString());
    if (it == customBlocks_.end())
        return;

    selectedCustomBlock_ = {it->first, it->second};
    refreshCustomButtonTextIcon();
}

BlockDataPair FallbackBlockSelector::fallbackBlock() const
{
    switch (mode_)
    {
        case NoneBlockMode:   return {std::string_view(), nullptr};
        case AirBlockMode:    return AIR_BLOCK_DATA_PAIR;
        case CustomBlockMode: return selectedCustomBlock_;
        default:              return {std::string_view(), nullptr};
    }
}

void FallbackBlockSelector::refreshText()
{
    noneBlockButton_->setText(EASYTR("None Block"));
    airBlockButton_->setText(EASYTR("Air Block"));
    customBlockSelectorButton_->setText(QStringLiteral("..."));
    refreshCustomButtonTextIcon();
}

void FallbackBlockSelector::refreshButtonState()
{
    switch (mode_)
    {
        case NoneBlockMode:
            noneBlockButton_->setChecked(true);
            break;
        case AirBlockMode:
            airBlockButton_->setChecked(true);
            break;
        case CustomBlockMode:
            customBlockButton_->setChecked(true);
            break;
        default:
            break;
    }
    customBlockSelectorButton_->setVisible(mode_ == CustomBlockMode);
}

void FallbackBlockSelector::refreshCustomButtonTextIcon()
{
    if (customBlocks_.count(selectedCustomBlock_.first) > 0)
    {
        const auto& textureFileName = customBlocks_.at(selectedCustomBlock_.first)->surfaceData.side.first;
        DirectoryScope dirScope(APP_RESOURCES_DIRPATH);
        const auto  texturePath = QStringLiteral("%1/%2")
            .arg(APP_BLOCK_TEXTURES_DIRPATH)
            .arg(QString::fromStdString(textureFileName));
        customBlockButton_->setIcon(QIcon(texturePath));
    }
    else
    {
        customBlockButton_->setIcon(QIcon());
    }

    const auto& entries = getAppConfiguration().blockEntries;
    const auto  blockId = std::string(selectedCustomBlock_.first);
    if (entries.count(blockId) > 0)
    {
        const auto& localizationNames = entries.at(blockId).localizationNames;

        QString blockName;
        if (localizationNames.count(easytr::currentLanguage()) > 0)
            blockName = QString::fromStdString(localizationNames.at(easytr::currentLanguage()));
        else
            blockName = QString::fromStdString(entries.at(blockId).name);
        customBlockButton_->setText(blockName);
    }
    else
    {
        customBlockButton_->setText(QString::fromStdString(blockId));
    }
}

#include "fallback_block_selector.moc"
