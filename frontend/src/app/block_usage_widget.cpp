#include "block_usage_widget.h"

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
    COLUMN_ID          = 0,
    COLUMN_NAME        = 1,
    COLUMN_COLOR       = 2,
    COLUMN_USAGE       = 3,
    COLUMN_COUNT       = 4
};

constexpr int ProgramBlockIdRole = Qt::UserRole + 1;
constexpr int BlockUsageRole     = Qt::UserRole + 2;

class BlockUsageItem : public QTableWidgetItem
{
public:
    using QTableWidgetItem::QTableWidgetItem;

    bool operator<(const QTableWidgetItem &other) const override
    {
        return data(BlockUsageRole).toULongLong() < other.data(BlockUsageRole).toULongLong();
    }
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

void BlockUsageWidget::setSortMode(SortMode mode)
{
    if (sortMode_ != mode)
    {
        sortMode_ = mode;
        sort();
        emit sortModeChanged(sortMode_);
    }
}

BlockUsageWidget::SortMode BlockUsageWidget::sortMode() const
{
    return sortMode_;
}

void BlockUsageWidget::sort()
{
    switch (sortMode_)
    {
        case IdAscendingSortMode:
            tableWidget_->sortItems(COLUMN_ID, Qt::AscendingOrder);
            break;
        case IdDescendingSortMode:
            tableWidget_->sortItems(COLUMN_ID, Qt::DescendingOrder);
            break;
        case UsageAscendingSortMode:
            tableWidget_->sortItems(COLUMN_USAGE, Qt::AscendingOrder);
            break;
        case UsageDescendingSortMode:
            tableWidget_->sortItems(COLUMN_USAGE, Qt::DescendingOrder);
            break;
        default:
            break;
    }
}

QString BlockUsageWidget::toMarkdown() const
{
    auto markdown = QStringLiteral("| %1 | %2 | %3 | %4 |\n| --- | --- | --- | --- |\n")
        .arg(EASYTR("Block ID"))
        .arg(EASYTR("Block Name"))
        .arg(EASYTR("Block Color"))
        .arg(EASYTR("Block Usage"));

    for (int row = 0; row < tableWidget_->rowCount(); ++row)
    {
        markdown += QStringLiteral("| %1 | %2 | %3 | %4 |\n")
            .arg(escapeMarkdownCell(tableWidget_->item(row, COLUMN_ID)->text()))
            .arg(escapeMarkdownCell(tableWidget_->item(row, COLUMN_NAME)->text()))
            .arg(escapeMarkdownCell(tableWidget_->item(row, COLUMN_COLOR)->text()))
            .arg(escapeMarkdownCell(tableWidget_->item(row, COLUMN_USAGE)->text()));
    }

    return markdown.trimmed();
}

void BlockUsageWidget::refreshText()
{
    updateHeaderLabels();

    const auto& config  = AppConfiguration::instance();
    const auto& entries = config.blockEntries;
    for (int row = 0; row < tableWidget_->rowCount(); ++row)
    {
        const auto programId = tableWidget_->item(row, COLUMN_ID)->data(ProgramBlockIdRole).toString().toStdString();
        const auto entryIt   = entries.find(programId);
        const auto blockIt   = blocks_.find(std::string_view(programId));
        const auto entry     = (entryIt == entries.end() ? nullptr : &entryIt->second);
        const auto block     = (blockIt == blocks_.end() ? nullptr : blockIt->second);

        const auto id = block ? QString::fromStdString(block->id) : QString();
        QString name;
        if (entry)
        {
            const auto& localizationNames = entry->localizationNames;
            const auto  nameIt            = localizationNames.find(easytr::currentLanguage());
            if (nameIt != localizationNames.end())
                name = QString::fromStdString(nameIt->second);
            else
                name = QString::fromStdString(entry->name);
        }
        else
        {
            name = id;
        }
        tableWidget_->item(row, COLUMN_NAME)->setText(name);
    }
}

void BlockUsageWidget::rebuildTableWidget()
{
    tableWidget_->clearContents();
    tableWidget_->setRowCount(blockUsage_.size());

    int row = 0;
    const auto& config  = AppConfiguration::instance();
    const auto& entries = config.blockEntries;
    for (const auto& [programId, usage] : blockUsage_)
    {
        const auto entryIt = entries.find(programId);
        const auto blockIt = blocks_.find(std::string_view(programId));
        const auto entry   = (entryIt == entries.end() ? nullptr : &entryIt->second);
        const auto block   = (blockIt == blocks_.end() ? nullptr : blockIt->second);

        const auto id     = block ? QString::fromStdString(block->id) : QString();
        auto*      idItem = new QTableWidgetItem(id);
        idItem->setData(ProgramBlockIdRole, QString::fromStdString(programId));

        QIcon blockIcon;
        if (block)
        {
            const auto textureName = block->surfaceData.get(surfaceDirection_).first;
            if (!textureName.empty())
            {
                DirectoryScope dirScope(APP_RESOURCES_DIRPATH);
                const auto texturePath = QStringLiteral("%1/%2")
                    .arg(APP_BLOCK_TEXTURES_DIRPATH)
                    .arg(QString::fromStdString(textureName));
                blockIcon = QIcon(texturePath);
            }
        }
        idItem->setIcon(blockIcon);

        QString name;
        if (entry)
        {
            const auto& localizationNames = entry->localizationNames;
            const auto  nameIt            = localizationNames.find(easytr::currentLanguage());
            if (nameIt != localizationNames.end())
                name = QString::fromStdString(nameIt->second);
            else
                name = QString::fromStdString(entry->name);
        }
        else
        {
            name = id;
        }
        auto* nameItem = new QTableWidgetItem(name);

        QString colorText;
        QIcon   colorIcon;
        if (block)
        {
            const auto color = block->surfaceData.get(surfaceDirection_).second;
            colorText = QString::fromStdString(color.toHex());
            colorIcon = colorPreviewIcon(color);
        }
        auto* colorItem = new QTableWidgetItem(colorText);
        colorItem->setIcon(colorIcon);

        auto* usageItem = new BlockUsageItem(QString::number(usage));
        usageItem->setData(BlockUsageRole, static_cast<qulonglong>(usage));

        tableWidget_->setItem(row, COLUMN_ID,    idItem);
        tableWidget_->setItem(row, COLUMN_NAME,  nameItem);
        tableWidget_->setItem(row, COLUMN_COLOR, colorItem);
        tableWidget_->setItem(row, COLUMN_USAGE, usageItem);
        ++row;
    }

    sort();
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

QIcon BlockUsageWidget::colorPreviewIcon(Rgb color)
{
    QPixmap pixmap(16, 16);
    pixmap.fill(QColor(color.r, color.g, color.b));
    return QIcon(pixmap);
}
