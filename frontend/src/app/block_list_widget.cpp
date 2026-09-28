#include "block_list_widget.h"

#include <qalgorithms.h>
#include <qboxlayout.h>
#include <qevent.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qscrollarea.h>

#include <config.h>
#include <components/flow_grid_layout.h>
#include <utils/directory_scope.h>

namespace
{

constexpr int ITEM_MARGIN          = 4;
constexpr int ITEM_SPACING         = 6;
constexpr int ITEM_INNER_GAP       = 4;
constexpr int ITEM_MIN_SIZE        = 72;
constexpr int ITEM_MAX_SIZE        = 108;
constexpr int ITEM_TEXT_HEIGHT     = 16;
constexpr int ITEM_PRIMARY_ALPHA   = 192;
constexpr int ITEM_SECONDARY_ALPHA = 96;

} // namespace

class BlockListItemWidget : public TrWidget
{
    Q_OBJECT

public:
    explicit BlockListItemWidget(const BlockEntryPair& entry, QWidget* parent = nullptr)
        : TrWidget(parent), entry_(entry)
    {
        if (entry_.second)
        {
            DirectoryScope dirScope(APP_RESOURCES_DIRPATH);
            const auto& baseBlock = entry_.second->baseBlock;
            const QString texturePath = QString("%1/%2")
                .arg(APP_BLOCK_TEXTURES_DIRPATH)
                .arg(QString::fromStdString(baseBlock.surfaceData.side.first));
            textureRaw_ = QPixmap(texturePath);
        }

        setContentsMargins(ITEM_MARGIN, ITEM_MARGIN, ITEM_MARGIN, ITEM_MARGIN);
        setMinimumSize(ITEM_MIN_SIZE, ITEM_MIN_SIZE);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        setCursor(Qt::PointingHandCursor);

        updateText();
    }

    const BlockEntryPair& entry() const
    {
        return entry_;
    }

    bool isSelected() const
    {
        return selected_;
    }

    void setSelected(bool selected)
    {
        if (selected_ != selected)
        {
            selected_ = selected;
            update();
        }
    }

    QSize sizeHint() const override
    {
        return QSize(ITEM_MIN_SIZE, ITEM_MIN_SIZE);
    }

signals:
    void clicked();

protected:
    void enterEvent(QEnterEvent* e) override
    {
        hovered_ = true;
        update();
    }

    void leaveEvent(QEvent*) override
    {
        hovered_ = false;
        update();
    }

    void mousePressEvent(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton)
            pressed_ = true;
        TrWidget::mousePressEvent(e);
    }

    void mouseReleaseEvent(QMouseEvent* e) override
    {
        if (pressed_ && e->button() == Qt::LeftButton && rect().contains(e->pos()))
            emit clicked();
        pressed_ = false;
        TrWidget::mouseReleaseEvent(e);
    }

    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        const QColor highlight = palette().color(QPalette::Highlight);
        const QMargins margins = contentsMargins();
        const QRect effectiveRect = rect().adjusted(margins.left(), margins.top(), -margins.right(), -margins.bottom());
        const QRectF cardRect  = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

        if (selected_ || hovered_)
        {
            QColor borderColor = highlight;
            QColor fillColor   = highlight;
            borderColor.setAlpha(selected_ ? 255 : ITEM_PRIMARY_ALPHA);
            fillColor.setAlpha(selected_ ? ITEM_PRIMARY_ALPHA : ITEM_SECONDARY_ALPHA);
            painter.setPen(borderColor);
            painter.setBrush(fillColor);
            painter.drawRoundedRect(cardRect, 6, 6);
        }

        const int textureSize    = qMax(1, effectiveRect.width()  / 2);
        const int colorBarHeight = qMax(1, effectiveRect.height() / 12);
        const int textureX = margins.left() + (effectiveRect.width() - textureSize) / 2;
        const int textureY = margins.top() + qMax(
            0,
            (effectiveRect.height() - textureSize - ITEM_TEXT_HEIGHT - colorBarHeight - ITEM_INNER_GAP * 2) / 2
        );

        const QRect textureRect(textureX, textureY, textureSize, textureSize);
        if (!textureRaw_.isNull())
        {
            if (textureScaledSize_ != textureSize)
            {
                textureScaledSize_ = textureSize;
                textureScaled_ = textureRaw_.scaled(
                    textureScaledSize_, textureScaledSize_,
                    Qt::IgnoreAspectRatio, Qt::FastTransformation
                );
            }
            painter.drawPixmap(textureRect, textureScaled_);
        }

        const QRect colorBarRect(
            textureRect.left(), textureRect.bottom() + ITEM_INNER_GAP,
            textureRect.width(), colorBarHeight
        );
        if (entry_.second)
        {
            const auto& baseBlock = entry_.second->baseBlock;
            const auto& color     = baseBlock.surfaceData.side.second;
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(color.r, color.g, color.b));
            painter.drawRect(colorBarRect);
        }

        const QRect textRect(
            effectiveRect.left(), colorBarRect.bottom() + ITEM_INNER_GAP,
            effectiveRect.width(), ITEM_TEXT_HEIGHT
        );
        if (!name_.isEmpty())
        {
            QFont f = font();
            f.setPointSizeF(9.0);
            painter.setFont(f);
            painter.setPen(palette().color(QPalette::WindowText));
            const QString& elided = QFontMetrics(f).elidedText(name_, Qt::ElideRight, textRect.width());
            painter.drawText(textRect, Qt::AlignCenter, elided);
            painter.setFont(font());
        }
    }

    void updateText() override
    {
        if (entry_.second)
        {
            const auto& localizationNames = entry_.second->localizationNames;
            auto it = localizationNames.find(easytr::currentLanguage());
            if (it != localizationNames.end())
                name_ = QString::fromStdString(it->second);
            else
                name_ = QString::fromStdString(entry_.second->name);

            const QString tooltipText = QString("%1: %2\n%3: %4\n%5: %6")
                .arg(EASYTR("Block Name")).arg(name_)
                .arg(EASYTR("Block ID")).arg(entry_.second->baseBlock.id)
                .arg(EASYTR("Minimum Version")).arg(QString::fromStdString(entry_.second->minVersion.toString())
            );
            setToolTip(tooltipText);
        }

        update();
    }

private:
    BlockEntryPair entry_;
    QString        name_;
    QPixmap        textureRaw_;
    QPixmap        textureScaled_;
    int            textureScaledSize_ = -1;

    bool selected_ = false;
    bool hovered_  = false;
    bool pressed_  = false;
};

BlockListWidget::BlockListWidget(const BlockEntryMap& entries, QWidget* parent)
    : TrWidget(parent), entries_(entries)
{
    gridContainer_ = new QWidget();
    gridLayout_    = new FlowGridLayout(ITEM_SPACING, ITEM_SPACING, ITEM_MIN_SIZE, ITEM_MAX_SIZE, gridContainer_);

    scrollArea_ = new QScrollArea();
    scrollArea_->setWidget(gridContainer_);
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scrollArea_);

    rebuildBlocks();
}

void BlockListWidget::select(const QString& blockId)
{
    setItemSelected(blockId, true);
}

void BlockListWidget::select(const QList<QString>& blockIds)
{
    for (const auto& blockId : blockIds)
        setItemSelected(blockId, true);
}

void BlockListWidget::unselect(const QString& blockId)
{
    setItemSelected(blockId, false);
}

void BlockListWidget::unselect(const QList<QString>& blockIds)
{
    for (const auto& blockId : blockIds)
        setItemSelected(blockId, false);
}

void BlockListWidget::selectAll()
{
    selecteds_ = blocks_;
    for (auto* item : items_)
        item->setSelected(true);
}

void BlockListWidget::unselectAll()
{
    selecteds_.clear();
    for (auto* item : items_)
        item->setSelected(false);
}

Version BlockListWidget::minimumVersion() const
{
    return minimumVersion_;
}

void BlockListWidget::setMinimumVersion(const Version& version)
{
    if (minimumVersion_ != version)
    {
        minimumVersion_ = version;
        rebuildBlocks();
    }
}

bool BlockListWidget::isMultiSelectEnabled() const
{
    return multiSelectEnabled_;
}

void BlockListWidget::setMultiSelectEnabled(bool enabled)
{
    if (multiSelectEnabled_ != enabled)
        multiSelectEnabled_ = enabled;

    if (!multiSelectEnabled_ && selecteds_.size() > 1)
        unselectAll();
}

void BlockListWidget::selectByAttribute(BlockAttributeFilterMode mode, const BlockAttributes& attributes)
{
    selecteds_ = filterBlocks(blocks_, mode, attributes);

    if (!multiSelectEnabled_ && selecteds_.size() > 1)
        selecteds_ = {*selecteds_.begin()};

    for (auto it = items_.constBegin(); it != items_.constEnd(); ++it)
    {
        const std::string id = it.key().toStdString();
        const bool selected = selecteds_.find(std::string_view(id)) != selecteds_.end();
        it.value()->setSelected(selected);
    }
}

void BlockListWidget::rebuildBlocks()
{
    blocks_    = resolveBlockEntries(entries_, minimumVersion_);
    selecteds_ = blocks_;
    rebuildItems();
}

void BlockListWidget::rebuildItems()
{
    qDeleteAll(items_);
    items_.clear();

    for (const auto& [id, block] : blocks_)
    {
        const std::string sid = std::string(id);
        const QString qid = QString::fromStdString(sid);

        const auto entryIt = entries_.find(sid);
        const BlockEntry* entry = entryIt != entries_.end() ? &entryIt->second : nullptr;

        auto* item = new BlockListItemWidget(BlockEntryPair(id, entry), gridContainer_);
        item->setSelected(selecteds_.count(id) > 0);

        connect(item, &BlockListItemWidget::clicked, this, [this, qid, item]()
        { setItemSelected(qid, !item->isSelected()); });

        gridLayout_->addWidget(item);
        items_.insert(qid, item);
    }
}

void BlockListWidget::setItemSelected(const QString& blockId, bool selected)
{
    const std::string id = blockId.toStdString();
    const auto blockIt = blocks_.find(std::string_view(id));
    if (blockIt == blocks_.end())
        return;

    if (selected)
    {
        if (!multiSelectEnabled_)
            unselectAll();
        selecteds_[blockIt->first] = blockIt->second;
    }
    else
    {
        selecteds_.erase(blockIt->first);
    }

    const auto itemIt = items_.find(blockId);
    if (itemIt != items_.end())
        itemIt.value()->setSelected(selected);
}

#include "block_list_widget.moc"
