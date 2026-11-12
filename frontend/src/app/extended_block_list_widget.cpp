#include "extended_block_list_widget.h"

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qgridlayout.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qvariantanimation.h>

#include <components/no_wheel_focus_filter.h>
#include "block_attribute_text.h"
#include "block_list_widget.h"

namespace
{

constexpr BlockAttribute ALL_BLOCK_ATTRIBUTES[] = {
    BLOCK_ATTRI_IS_INCOMPLETE,
    BLOCK_ATTRI_IS_TRANSPARENT,
    BLOCK_ATTRI_IS_LUMINOUS,
    BLOCK_ATTRI_IS_UNSTABLE,
    BLOCK_ATTRI_IS_CREATIVE,
    BLOCK_ATTRI_HAS_GRAVITY,
    BLOCK_ATTRI_HAS_PATTERN,
    BLOCK_ATTRI_FLAMMABLE,
    BLOCK_ATTRI_ENDERMAN_PICKABLE
};

constexpr int FILTER_ATTRIBUTE_COLUMNS      = 3;
constexpr int FILTER_PANEL_ANIM_DURATION_MS = 200;

} // namespace

ExtendedBlockListWidget::ExtendedBlockListWidget(QWidget* parent)
    : AdaptiveWidget(parent)
{
    // 构建顶部工具栏 UI。
    selectAllButton_     = new QPushButton(this);
    unselectAllButton_   = new QPushButton(this);
    selectionCountLabel_ = new QLabel(this);
    filterToggleButton_  = new QPushButton(this);
    filterToggleButton_->setCheckable(true);

    auto* toolbarLayout = new QHBoxLayout();
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(3);
    toolbarLayout->addWidget(selectAllButton_);
    toolbarLayout->addWidget(unselectAllButton_);
    toolbarLayout->addStretch(1);
    toolbarLayout->addWidget(selectionCountLabel_);
    toolbarLayout->addWidget(filterToggleButton_);

    // 构建可展开/折叠的方块属性筛选面板。
    filterPanel_ = new QWidget(this);
    filterPanel_->setMaximumHeight(0);

    // 方块属性可选框。
    auto* attributesLayout = new QGridLayout();
    attributesLayout->setContentsMargins(0, 2, 0, 2);
    for (std::size_t i = 0; i < std::size(ALL_BLOCK_ATTRIBUTES); ++i)
    {
        auto* checkBox = new QCheckBox(filterPanel_);
        checkBox->setProperty("blockAttribute", static_cast<int>(ALL_BLOCK_ATTRIBUTES[i]));
        attributeCheckBoxes_.append(checkBox);
        attributesLayout->addWidget(
            checkBox,
            static_cast<int>(i) / FILTER_ATTRIBUTE_COLUMNS,
            static_cast<int>(i) % FILTER_ATTRIBUTE_COLUMNS
        );

        connect(
            checkBox, &QCheckBox::toggled,
            this, [this]() { emit checkedBlockAttributesChanged(checkedBlockAttributes()); }
        );
    }

    // 方块属性筛选模式。
    filterModeLabel_ = new QLabel(filterPanel_);
    filterModeCombo_ = new QComboBox(filterPanel_);
    QT_BAN_WHEEL_FOCUS(filterModeCombo_);
    for (const auto mode : {BLOCK_ATTRI_FILTER_MODE_CONTAINS_ANY, BLOCK_ATTRI_FILTER_MODE_CONTAINS_ALL})
        filterModeCombo_->addItem(QString(), static_cast<int>(mode));

    // 筛选操作。
    excludeButton_    = new QPushButton(filterPanel_);
    supplementButton_ = new QPushButton(filterPanel_);

    auto* filterActionLayout = new QHBoxLayout();
    filterActionLayout->setContentsMargins(0, 0, 0, 0);
    filterActionLayout->setSpacing(3);
    filterActionLayout->addWidget(filterModeLabel_);
    filterActionLayout->addWidget(filterModeCombo_, 1);
    filterActionLayout->addWidget(excludeButton_);
    filterActionLayout->addWidget(supplementButton_);

    auto* filterPanelLayout = new QVBoxLayout(filterPanel_);
    filterPanelLayout->setContentsMargins(0, 0, 0, 0);
    filterPanelLayout->setSpacing(0);
    filterPanelLayout->addLayout(attributesLayout);
    filterPanelLayout->addLayout(filterActionLayout);

    // 主布局。
    blockListWidget_ = new BlockListWidget(this);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addLayout(toolbarLayout);
    mainLayout->addWidget(filterPanel_);
    mainLayout->addWidget(blockListWidget_, 1);

    // 筛选面板的展开/折叠动画。
    filterPanelAnimation_ = new QVariantAnimation(this);
    filterPanelAnimation_->setDuration(FILTER_PANEL_ANIM_DURATION_MS);
    filterPanelAnimation_->setEasingCurve(QEasingCurve::InOutCubic);
    connect(filterPanelAnimation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
    { filterPanel_->setMaximumHeight(value.toInt()); });

    connect(selectAllButton_,    &QPushButton::clicked, blockListWidget_, &BlockListWidget::selectAll);
    connect(unselectAllButton_,  &QPushButton::clicked, blockListWidget_, &BlockListWidget::unselectAll);
    connect(filterToggleButton_, &QPushButton::toggled, this,             &ExtendedBlockListWidget::toggleFilterPanel);
    connect(excludeButton_,      &QPushButton::clicked, this,             [this]() { applyFilter(false); });
    connect(supplementButton_,   &QPushButton::clicked, this,             [this]() { applyFilter(true);  });
    connect(
        filterModeCombo_, &QComboBox::currentIndexChanged,
        this, [this]() { emit blockFilterModeChanged(blockFilterMode()); }
    );
    connect(
        filterToggleButton_, &QPushButton::clicked,
        this, [this]() { emit filterPanelExpandedToggled(filterPanelExpanded()); }
    );
    connect(
        blockListWidget_, &BlockListWidget::selectionChanged,
        this, [this]() { refreshSelectionCountLabel(); emit selectionChanged(); }
    );

    refreshText();
    refreshSelectionCountLabel();
}

void ExtendedBlockListWidget::select(const QString& blockId)
{
    blockListWidget_->select(blockId);
}

void ExtendedBlockListWidget::select(const QList<QString>& blockIds)
{
    blockListWidget_->select(blockIds);
}

void ExtendedBlockListWidget::unselect(const QString& blockId)
{
    blockListWidget_->unselect(blockId);
}

void ExtendedBlockListWidget::unselect(const QList<QString>& blockIds)
{
    blockListWidget_->unselect(blockIds);
}

void ExtendedBlockListWidget::selectAll()
{
    blockListWidget_->selectAll();
}

void ExtendedBlockListWidget::unselectAll()
{
    blockListWidget_->unselectAll();
}

void ExtendedBlockListWidget::setCheckedBlockAttributes(int blockAttributes)
{
    if (checkedBlockAttributes() == blockAttributes)
        return;

    for (auto& box : attributeCheckBoxes_)
    {
        const bool old = box->blockSignals(true);
        box->setChecked((box->property("blockAttribute").toInt() & blockAttributes) != 0);
        box->blockSignals(old);
    }
    emit checkedBlockAttributesChanged(blockAttributes);
}

void ExtendedBlockListWidget::setBlockFilterMode(BlockAttributeFilterMode mode)
{
    for (int i = 0; i < filterModeCombo_->count(); ++i)
    {
        if (filterModeCombo_->itemData(i).toInt() == mode)
        {
            if (filterModeCombo_->currentIndex() != i)
            {
                const bool old = filterModeCombo_->blockSignals(true);
                filterModeCombo_->setCurrentIndex(i);
                filterModeCombo_->blockSignals(old);
                emit blockFilterModeChanged(mode);
            }
            break;
        }
    }
}

int ExtendedBlockListWidget::checkedBlockAttributes() const
{
    int attributes = 0;
    for (const auto& box : attributeCheckBoxes_)
    {
        if (box->isChecked())
            attributes |= box->property("blockAttribute").toInt();
    }
    return attributes;
}

BlockAttributeFilterMode ExtendedBlockListWidget::blockFilterMode() const
{
    for (int i = 0; i < filterModeCombo_->count(); ++i)
    {
        if (filterModeCombo_->currentIndex() == i)
            return static_cast<BlockAttributeFilterMode>(filterModeCombo_->itemData(i).toInt());
    }
    return BLOCK_ATTRI_FILTER_MODE_CONTAINS_ANY;
}

void ExtendedBlockListWidget::setFilterPanelExpanded(bool expanded)
{
    if (filterToggleButton_->isChecked() != expanded)
        filterToggleButton_->toggle();
}

bool ExtendedBlockListWidget::filterPanelExpanded() const
{
    return filterToggleButton_->isChecked();
}

void ExtendedBlockListWidget::setBlocks(BlockDataMap blocks)
{
    blockListWidget_->setBlocks(std::move(blocks));
    refreshSelectionCountLabel();
}

const BlockDataMap& ExtendedBlockListWidget::blocks() const
{
    return blockListWidget_->blocks();
}

const BlockDataMap& ExtendedBlockListWidget::selecteds() const
{
    return blockListWidget_->selecteds();
}

void ExtendedBlockListWidget::refreshText()
{
    selectAllButton_->setText(EASYTR("Select All"));
    unselectAllButton_->setText(EASYTR("Select None"));
    filterToggleButton_->setText(EASYTR("Filter"));
    filterModeLabel_->setText(EASYTR("Filter Mode:"));
    excludeButton_->setText(EASYTR("Exclude"));
    supplementButton_->setText(EASYTR("Supplement"));

    for (auto* checkBox : attributeCheckBoxes_)
    {
        const auto attribute = static_cast<BlockAttribute>(checkBox->property("blockAttribute").toInt());
        checkBox->setText(makeBlockAttributesText(attribute));
    }

    const int currentIndex = filterModeCombo_->currentIndex();
    for (int i = 0; i < filterModeCombo_->count(); ++i)
    {
        const auto mode = static_cast<BlockAttributeFilterMode>(filterModeCombo_->itemData(i).toInt());
        filterModeCombo_->setItemText(i, getBlockAttributeFilterModeText(mode));
    }
    filterModeCombo_->setCurrentIndex(currentIndex);

    refreshSelectionCountLabel();
}

BlockAttributes ExtendedBlockListWidget::checkedAttributes() const
{
    BlockAttributes attributes = BLOCK_ATTRI_NONE;
    for (const auto* checkBox : attributeCheckBoxes_)
    {
        if (checkBox->isChecked())
            attributes |= checkBox->property("blockAttribute").toInt();
    }
    return attributes;
}

void ExtendedBlockListWidget::applyFilter(bool supplement)
{
    const auto mode       = static_cast<BlockAttributeFilterMode>(filterModeCombo_->currentData().toInt());
    const auto attributes = checkedAttributes();

    if (supplement) blockListWidget_->appendBlocks(attributes, mode);
    else            blockListWidget_->excludeBlocks(attributes, mode);
}

void ExtendedBlockListWidget::refreshSelectionCountLabel()
{
    selectionCountLabel_->setText(
        QStringLiteral("%1: %2 / %3")
            .arg(EASYTR("Selected"))
            .arg(blockListWidget_->selecteds().size())
            .arg(blockListWidget_->blocks().size())
    );
}

void ExtendedBlockListWidget::toggleFilterPanel(bool expand)
{
    const int targetHeight = expand ? filterPanel_->sizeHint().height() : 0;

    filterPanelAnimation_->stop();
    filterPanelAnimation_->setStartValue(filterPanel_->height());
    filterPanelAnimation_->setEndValue(targetHeight);
    filterPanelAnimation_->start();
}
