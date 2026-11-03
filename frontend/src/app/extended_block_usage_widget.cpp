#include "extended_block_usage_widget.h"

#include <qcombobox.h>
#include <qfiledialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtoolbutton.h>

#include <utils/debug_output.h>
#include <utils/file_io.h>

ExtendedBlockUsageWidget::ExtendedBlockUsageWidget(QWidget* parent)
    : AdaptiveWidget(parent)
{
    comboBoxLabel_ = new QLabel(this);

    comboBox_ = new QComboBox(this);
    comboBox_->addItems({"", "", "", ""});
    comboBox_->setItemData(0, static_cast<int>(BlockUsageWidget::IdAscendingSortMode));
    comboBox_->setItemData(1, static_cast<int>(BlockUsageWidget::IdDescendingSortMode));
    comboBox_->setItemData(2, static_cast<int>(BlockUsageWidget::UsageAscendingSortMode));
    comboBox_->setItemData(3, static_cast<int>(BlockUsageWidget::UsageDescendingSortMode));

    exportButton_ = new QToolButton(this);

    widget_ = new BlockUsageWidget(this);

    auto* topWidget = new QWidget(this);
    auto* topLayout = new QHBoxLayout(topWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(2);
    topLayout->addWidget(comboBoxLabel_);
    topLayout->addWidget(comboBox_);
    topLayout->addStretch();
    topLayout->addWidget(exportButton_);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(topWidget);
    mainLayout->addWidget(widget_);

    connect(comboBox_,     &QComboBox::currentIndexChanged,    this, &ExtendedBlockUsageWidget::onComboBoxIndexChanged);
    connect(exportButton_, &QToolButton::clicked,              this, &ExtendedBlockUsageWidget::onExportButtonClicked);
    connect(widget_,       &BlockUsageWidget::sortModeChanged, this, &ExtendedBlockUsageWidget::onSortModeChanged);

    refreshText();
}

void ExtendedBlockUsageWidget::setMarkdownTitle(QString title)
{
    markdownTitle_ = title;
}

void ExtendedBlockUsageWidget::setMarkdownSummary(QString summary)
{
    markdownSummary_ = summary;
}

const QString& ExtendedBlockUsageWidget::markdownTitle() const
{
    return markdownTitle_;
}

const QString& ExtendedBlockUsageWidget::markdownSummary() const
{
    return markdownTitle_;
}

BlockUsageWidget* ExtendedBlockUsageWidget::blockUsageWidget() const
{
    return widget_;
}

QString ExtendedBlockUsageWidget::toMarkdown() const
{
    return QStringLiteral("# %1\n\n%2\n\n%3")
        .arg(markdownTitle_)
        .arg(markdownSummary_)
        .arg(widget_->toMarkdown())
        .trimmed();
}

void ExtendedBlockUsageWidget::refreshText()
{
    comboBoxLabel_->setText(EASYTR("Sort By"));
    for (int i = 0; i < 4; ++i)
        comboBox_->setItemText(i, sortModeText(static_cast<BlockUsageWidget::SortMode>(i)));
    exportButton_->setText(EASYTR("Export Markdown"));
}

QString ExtendedBlockUsageWidget::sortModeText(BlockUsageWidget::SortMode mode)
{
    switch (mode)
    {
        case BlockUsageWidget::IdAscendingSortMode:     return EASYTR("Block ID (Ascending)");
        case BlockUsageWidget::IdDescendingSortMode:    return EASYTR("Block ID (Descending)");
        case BlockUsageWidget::UsageAscendingSortMode:  return EASYTR("Block Usage (Ascending)");
        case BlockUsageWidget::UsageDescendingSortMode: return EASYTR("Block Usage (Descending)");
        default:                                        return QString();
    }
}

void ExtendedBlockUsageWidget::onComboBoxIndexChanged(int index)
{
    const auto sortMode = static_cast<BlockUsageWidget::SortMode>(comboBox_->itemData(index).toInt());
    const auto old      = widget_->blockSignals(true);
    widget_->setSortMode(sortMode);
    widget_->blockSignals(old);
}

void ExtendedBlockUsageWidget::onExportButtonClicked()
{
    const auto filePath = QFileDialog::getSaveFileName(
        this,
        EASYTR("Saved to:"),
        markdownTitle_ + QStringLiteral(".md"),
        QStringLiteral("%1 (*.md);; %2 (*)").arg(EASYTR("Markdown File")).arg(EASYTR("All File"))
    );
    if (filePath.isEmpty())
        return;

    try { writeFileContent(filePath, toMarkdown()); }
    catch (std::exception& e) { debugOut(qDebug(), "Export Markdown file go wrong: %1", e.what()); }
}

void ExtendedBlockUsageWidget::onSortModeChanged(BlockUsageWidget::SortMode mode)
{
    const auto old = comboBox_->blockSignals(true);
    comboBox_->setCurrentIndex(static_cast<int>(mode));
    comboBox_->blockSignals(old);
}
