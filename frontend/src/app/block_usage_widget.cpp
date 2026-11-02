#include "block_usage_widget.h"

#include <algorithm>

#include <qcolor.h>
#include <qheaderview.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qstringlist.h>
#include <qtablewidget.h>

#include <app_defines.h>
#include <utils/directory_scope.h>
#include "app_configuration.h"

namespace
{

enum
{
    COLUMN_ID    = 0,
    COLUMN_NAME  = 1,
    COLUMN_COLOR = 2,
    COLUMN_USAGE = 3,
    COLUMN_COUNT = 4
};

QString escapeMarkdownCell(QString text)
{
    return text
        .replace(QLatin1Char('|'), QStringLiteral("\\|"))
        .replace(QLatin1Char('\n'), QLatin1Char(' '));
}

} // namespace

BlockUsageWidget::BlockUsageWidget(QWidget* parent)
    : AdaptiveWidget(parent)
{
    tableWidget_ = new QTableWidget(this);
    tableWidget_->setColumnCount(COLUMN_COUNT);
    tableWidget_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget_->setSelectionMode(QAbstractItemView::NoSelection);
    tableWidget_->verticalHeader()->setVisible(false);
    tableWidget_->horizontalHeader()->setSectionResizeMode(COLUMN_ID,    QHeaderView::Stretch);
    tableWidget_->horizontalHeader()->setSectionResizeMode(COLUMN_NAME,  QHeaderView::ResizeToContents);
    tableWidget_->horizontalHeader()->setSectionResizeMode(COLUMN_COLOR, QHeaderView::ResizeToContents);
    tableWidget_->horizontalHeader()->setSectionResizeMode(COLUMN_USAGE, QHeaderView::ResizeToContents);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(tableWidget_);

    updateHeaderLabels();
}

void BlockUsageWidget::setBlocks(BlockDataMap blocks)
{
    blocks_ = std::move(blocks);
    rebuildTableWidget();
}

const BlockDataMap& BlockUsageWidget::blocks() const
{
    return blocks_;
}

void BlockUsageWidget::setSurfaceDirection(SurfaceDirection direction)
{
    if (surfaceDirection_ != direction)
    {
        surfaceDirection_ = direction;
        rebuildTableWidget();
    }
}

SurfaceDirection BlockUsageWidget::surfaceDirection() const
{
    return surfaceDirection_;
}

void BlockUsageWidget::setBlockUsage(BlockUsageType usage)
{
    blockUsage_ = std::move(usage);
    rebuildTableWidget();
}

const BlockUsageWidget::BlockUsageType& BlockUsageWidget::blockUsage() const
{
    return blockUsage_;
}

void BlockUsageWidget::sort(SortMode mode)
{
    sortMode_ = mode;
    sortRows();
    populateTable();
}

BlockUsageWidget::SortMode BlockUsageWidget::sortMode() const
{
    return sortMode_;
}

QString BlockUsageWidget::toMarkdown() const
{
    QString markdown = QStringLiteral("| %1 | %2 | %3 | %4 |\n| --- | --- | --- | --- |\n")
        .arg(EASYTR("Block ID"))
        .arg(EASYTR("Block Name"))
        .arg(EASYTR("Block Color"))
        .arg(EASYTR("Block Usage"));

    for (const auto& row : rows_)
    {
        markdown += QStringLiteral("| %1 | %2 | %3 | %4 |\n")
            .arg(escapeMarkdownCell(displayBlockId(row)))
            .arg(escapeMarkdownCell(localizedBlockName(row)))
            .arg(escapeMarkdownCell(colorHexText(row)))
            .arg(static_cast<qulonglong>(row.usage));
    }

    return markdown.trimmed();
}

void BlockUsageWidget::refreshText()
{
    updateHeaderLabels();

    const int rowCount = qMin(static_cast<int>(rows_.size()), tableWidget_->rowCount());
    for (int r = 0; r < rowCount; ++r)
    {
        if (auto* item = tableWidget_->item(r, COLUMN_NAME))
            item->setText(localizedBlockName(rows_[static_cast<std::size_t>(r)]));
    }
}

void BlockUsageWidget::rebuildTableWidget()
{
    rebuildRows();
    sortRows();
    populateTable();
}

void BlockUsageWidget::rebuildRows()
{
    rows_.clear();
    rows_.reserve(blockUsage_.size());

    for (const auto& [id, count] : blockUsage_)
    {
        RowData row;
        row.programId = id;
        const auto it  = blocks_.find(std::string_view(id));
        row.block      = (it != blocks_.end()) ? it->second : nullptr;
        row.usage      = count;
        rows_.push_back(std::move(row));
    }
}

void BlockUsageWidget::sortRows()
{
    switch (sortMode_)
    {
        case IdDescendingSortMode:
            std::sort(rows_.begin(), rows_.end(), [this](const RowData& a, const RowData& b)
            {
                return rawBlockId(a) > rawBlockId(b);
            });
            break;
        case UsageAscendingSortMode:
            std::sort(rows_.begin(), rows_.end(), [this](const RowData& a, const RowData& b)
            {
                if (a.usage != b.usage) return a.usage < b.usage;
                return rawBlockId(a) < rawBlockId(b);
            });
            break;
        case UsageDescendingSortMode:
            std::sort(rows_.begin(), rows_.end(), [this](const RowData& a, const RowData& b)
            {
                if (a.usage != b.usage) return a.usage > b.usage;
                return rawBlockId(a) < rawBlockId(b);
            });
            break;
        case IdAscendingSortMode: // Fallthrough
        default:
            std::sort(rows_.begin(), rows_.end(), [this](const RowData& a, const RowData& b)
            {
                return rawBlockId(a) < rawBlockId(b);
            });
            break;
    }
}

void BlockUsageWidget::populateTable()
{
    updateHeaderLabels();

    tableWidget_->clearContents();
    tableWidget_->setRowCount(static_cast<int>(rows_.size()));

    for (int r = 0; r < static_cast<int>(rows_.size()); ++r)
    {
        const RowData& row = rows_[static_cast<std::size_t>(r)];

        auto* idItem = new QTableWidgetItem(displayBlockId(row));
        idItem->setIcon(blockIcon(row));
        tableWidget_->setItem(r, COLUMN_ID, idItem);

        auto* nameItem = new QTableWidgetItem(localizedBlockName(row));
        tableWidget_->setItem(r, COLUMN_NAME, nameItem);

        auto* colorItem = new QTableWidgetItem(colorHexText(row));
        colorItem->setIcon(colorPreviewIcon(row));
        tableWidget_->setItem(r, COLUMN_COLOR, colorItem);

        auto* usageItem = new QTableWidgetItem(QString::number(row.usage));
        usageItem->setTextAlignment(Qt::AlignCenter);
        tableWidget_->setItem(r, COLUMN_USAGE, usageItem);
    }
}

void BlockUsageWidget::updateHeaderLabels()
{
    tableWidget_->setHorizontalHeaderLabels(QStringList{
        EASYTR("Block ID"),
        EASYTR("Block Name"),
        EASYTR("Block Color"),
        EASYTR("Block Usage")
    });
}

const std::string& BlockUsageWidget::rawBlockId(const RowData& row) const
{
    return row.block ? row.block->id : row.programId;
}

QString BlockUsageWidget::displayBlockId(const RowData& row) const
{
    return QString::fromStdString(rawBlockId(row));
}

QString BlockUsageWidget::localizedBlockName(const RowData& row) const
{
    const auto& entries = getAppConfiguration().blockEntries;
    const auto  entryIt = entries.find(row.programId);
    if (entryIt == entries.end())
        return displayBlockId(row);

    const auto& localizationNames = entryIt->second.localizationNames;
    const auto  nameIt = localizationNames.find(easytr::currentLanguage());
    if (nameIt != localizationNames.end())
        return QString::fromStdString(nameIt->second);

    return QString::fromStdString(entryIt->second.name);
}

QString BlockUsageWidget::colorHexText(const RowData& row) const
{
    if (!row.block)
        return QString();

    const Rgb& color = row.block->surfaceData.get(surfaceDirection_).second;
    return QString::fromStdString(color.toHex(color));
}

QIcon BlockUsageWidget::blockIcon(const RowData& row) const
{
    if (!row.block)
        return QIcon();

    const auto& texturePath = row.block->surfaceData.get(surfaceDirection_).first;
    if (texturePath.empty())
        return QIcon();

    DirectoryScope dirScope(APP_RESOURCES_DIRPATH);
    const auto& fullPath = QStringLiteral("%1/%2")
        .arg(APP_BLOCK_TEXTURES_DIRPATH)
        .arg(QString::fromStdString(texturePath));
    return QIcon(fullPath);
}

QIcon BlockUsageWidget::colorPreviewIcon(const RowData& row) const
{
    if (!row.block)
        return QIcon();

    const Rgb& color = row.block->surfaceData.get(surfaceDirection_).second;
    QPixmap pixmap(16, 16);
    pixmap.fill(QColor(color.r, color.g, color.b));
    return QIcon(pixmap);
}
