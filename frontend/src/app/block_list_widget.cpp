#include "block_list_widget.h"

#include <qalgorithms.h>
#include <qevent.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qscrollarea.h>

#include <config.h>
#include <components/flow_grid_layout.h>
#include <utils/directory_scope.h>
#include "block_attribute_text.h"

namespace
{

constexpr int ITEM_MARGIN          = 4;
constexpr int ITEM_SPACING         = 6;
constexpr int ITEM_INNER_GAP       = 6;
constexpr int ITEM_MIN_SIZE        = 64;
constexpr int ITEM_MAX_SIZE        = 96;
constexpr int ITEM_TEXT_HEIGHT     = 14;
constexpr int ITEM_PRIMARY_ALPHA   = 192;
constexpr int ITEM_SECONDARY_ALPHA = 96;

} // namespace

class BlockListItemWidget : public AdaptiveWidget
{
    Q_OBJECT

public:
    explicit BlockListItemWidget(const BlockEntryPair& entry, QWidget* parent = nullptr)
        : AdaptiveWidget(parent), entry_(entry)
    {
        if (entry_.second)
        {
            DirectoryScope dirScope(APP_RESOURCES_DIRPATH);
            const auto& baseBlock   = entry_.second->baseBlock;
            const auto& texturePath = QStringLiteral("%1/%2")
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

    void setSelected(bool selected)
    {
        if (selected_ != selected)
        {
            selected_ = selected;
            update();
        }
    }

    bool isSelected() const
    {
        return selected_;
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
        AdaptiveWidget::mousePressEvent(e);
    }

    void mouseReleaseEvent(QMouseEvent* e) override
    {
        if (pressed_ && e->button() == Qt::LeftButton && rect().contains(e->pos()))
            emit clicked();
        pressed_ = false;
        AdaptiveWidget::mouseReleaseEvent(e);
    }

    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        const auto& highlight     = palette().color(QPalette::Highlight);
        const auto& margins       = contentsMargins();
        const auto& effectiveRect = rect().adjusted(margins.left(), margins.top(), -margins.right(), -margins.bottom());
        const auto& cardRect      = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

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
        else
        {
            QColor fillColor = palette().color(QPalette::Base);
            fillColor.setAlpha(ITEM_PRIMARY_ALPHA);
            painter.setPen(Qt::NoPen);
            painter.setBrush(fillColor);
            painter.drawRoundedRect(cardRect, 6, 6);
        }

        const int textureSize    = qMax(1, effectiveRect.width()  / 2);
        const int colorBarHeight = qMax(1, effectiveRect.height() / 12);
        const int textureX       = margins.left() + (effectiveRect.width() - textureSize) / 2;
        const int textureY       = margins.top() + qMax(
            0,
            (effectiveRect.height() - textureSize - ITEM_TEXT_HEIGHT - colorBarHeight - ITEM_INNER_GAP * 2) / 2
        );

        const QRect textureRect(textureX, textureY, textureSize, textureSize);
        if (!textureRaw_.isNull())
        {
            if (textureScaledSize_ != textureSize)
            {
                textureScaledSize_ = textureSize;
                textureScaled_     = textureRaw_.scaled(
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
            f.setPointSizeF(f.pointSizeF() * 0.8);
            painter.setFont(f);
            painter.setPen(palette().color(QPalette::WindowText));
            const auto& elided = QFontMetrics(f).elidedText(name_, Qt::ElideRight, textRect.width());
            painter.drawText(textRect, Qt::AlignCenter, elided);
            painter.setFont(font());
        }
    }

    void updateText(LanguageManager::Language loaded = LanguageManager::instance().loadedLanguage()) override
    {
        (void)loaded;

        if (entry_.second)
        {
            const auto& localizationNames = entry_.second->localizationNames;
            auto it = localizationNames.find(easytr::currentLanguage());
            if (it != localizationNames.end())
                name_ = QString::fromStdString(it->second);
            else
                name_ = QString::fromStdString(entry_.second->name);

            auto attributesText = makeBlockAttributesText(entry_.second->baseBlock.attributes);
            attributesText = QFontMetrics(font()).elidedText(attributesText, Qt::ElideRight, 480);
            const auto& tooltipText = QStringLiteral("%1: %2\n%3: %4\n%5: %6\n%7: %8")
                .arg(EASYTR("Block Name")).arg(name_)
                .arg(EASYTR("Block ID")).arg(entry_.second->baseBlock.id)
                .arg(EASYTR("Block Attributes")).arg(attributesText)
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
    : AdaptiveWidget(parent), entries_(entries)
{
    gridContainer_ = new QWidget();
    gridLayout_    = new FlowGridLayout(ITEM_SPACING, ITEM_SPACING, ITEM_MIN_SIZE, ITEM_MAX_SIZE, gridContainer_);

    scrollArea_ = new QScrollArea();
    scrollArea_->setWidget(gridContainer_);
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(scrollArea_);

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

void BlockListWidget::selectRange(int start, int end)
{
    if (start > end || start < 0 || end >= static_cast<int>(items_.size()))
        return;

    if (!multiSelectEnabled_)
        return;

    auto it = items_.begin();
    for (; start <= end; ++start)
    {
        const auto& id = std::next(it, start).key().toStdString();
        std::next(it, start).value()->setSelected(true);
        if (selecteds_.find(std::string_view(id)) == selecteds_.end())
            selecteds_[std::string_view(id)] = blocks_[std::string_view(id)];
    }

    emit selectionChanged();
}

void BlockListWidget::unselectRange(int start, int end)
{
    if (start > end || start < 0 || end >= static_cast<int>(items_.size()))
        return;

    auto it = items_.begin();
    for (; start <= end; ++start)
    {
        const auto& id = std::next(it, start).key().toStdString();
        std::next(it, start).value()->setSelected(false);
        if (selecteds_.find(std::string_view(id)) != selecteds_.end())
            selecteds_.erase(std::string_view(id));
    }

    emit selectionChanged();
}

void BlockListWidget::selectAll()
{
    if (!multiSelectEnabled_)
        return;

    selecteds_ = blocks_;
    for (auto* item : items_)
        item->setSelected(true);

    emit selectionChanged();
}

void BlockListWidget::unselectAll()
{
    selecteds_.clear();
    for (auto* item : items_)
        item->setSelected(false);

    emit selectionChanged();
}

void BlockListWidget::setMinimumVersion(const Version& version)
{
    if (minimumVersion_ != version)
    {
        minimumVersion_ = version;
        rebuildBlocks();
    }
}

Version BlockListWidget::minimumVersion() const
{
    return minimumVersion_;
}

void BlockListWidget::setMultiSelectEnabled(bool enabled)
{
    if (multiSelectEnabled_ != enabled)
        multiSelectEnabled_ = enabled;

    if (!multiSelectEnabled_ && selecteds_.size() > 1)
        unselectAll();
}

bool BlockListWidget::multiSelectEnabled() const
{
    return multiSelectEnabled_;
}

void BlockListWidget::excludeBlocks(const BlockAttributes& attributes, BlockAttributeFilterMode filterMode)
{
    ::excludeBlocks(selecteds_, attributes, filterMode);
    onSelectedsChanged();
    emit selectionChanged();
}

void BlockListWidget::appendBlocks(const BlockAttributes& attributes, BlockAttributeFilterMode filterMode)
{
    ::appendBlocks(selecteds_, blocks_, attributes, filterMode);
    onSelectedsChanged();
    emit selectionChanged();
}

const BlockDataMap& BlockListWidget::blocks() const
{
    return blocks_;
}

const BlockDataMap& BlockListWidget::selecteds() const
{
    return selecteds_;
}

void BlockListWidget::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Shift)
        shiftPressed_ = true;
    AdaptiveWidget::keyPressEvent(e);
}

void BlockListWidget::keyReleaseEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Shift)
    {
        rangeSelectStart_ = -1;
        rangeSelectEnd_   = -1;
        shiftPressed_     = false;
    }
    AdaptiveWidget::keyReleaseEvent(e);
}

void BlockListWidget::rebuildBlocks()
{
    blocks_    = resolveBlockEntries(entries_, minimumVersion_);
    selecteds_ = blocks_;
    rebuildItems();

    emit selectionChanged();
}

void BlockListWidget::rebuildItems()
{
    qDeleteAll(items_);
    items_.clear();

    int index = 0;
    for (const auto& [id, block] : blocks_)
    {
        const auto& sid = std::string(id);
        const auto& qid = QString::fromStdString(sid);

        const auto  entryIt = entries_.find(sid);
        const auto* entry   = entryIt != entries_.end() ? &entryIt->second : nullptr;

        auto* item = new BlockListItemWidget(BlockEntryPair(id, entry), gridContainer_);
        item->setSelected(selecteds_.count(id) > 0);

        connect(item, &BlockListItemWidget::clicked, this, [this, qid, item, index]()
        {
            if (shiftPressed_ && multiSelectEnabled_)
            {
                if (rangeSelectStart_ < 0)
                {
                    rangeSelectStart_ = index;
                    startIsSelected_  = item->isSelected();
                    setItemSelected(qid, !item->isSelected());
                }
                else
                {
                    rangeSelectEnd_ = index;
                    const auto& begin = std::min(rangeSelectStart_, rangeSelectEnd_);
                    const auto& end   = std::max(rangeSelectStart_, rangeSelectEnd_);
                    if (startIsSelected_) unselectRange(begin, end);
                    else                selectRange(begin,   end);
                }
            }
            else
            {
                setItemSelected(qid, !item->isSelected());
            }
        });

        gridLayout_->addWidget(item);
        items_.insert(qid, item);
        ++index;
    }
}

void BlockListWidget::setItemSelected(const QString& blockId, bool selected)
{
    const auto& id      = blockId.toStdString();
    const auto  blockIt = blocks_.find(std::string_view(id));
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

    emit selectionChanged();
}

void BlockListWidget::onSelectedsChanged()
{
    if (!multiSelectEnabled_ && selecteds_.size() > 1)
    selecteds_ = {*selecteds_.begin()};

    for (auto it = items_.constBegin(); it != items_.constEnd(); ++it)
    {
        const auto& id       = it.key().toStdString();
        const bool  selected = selecteds_.find(std::string_view(id)) != selecteds_.end();
        it.value()->setSelected(selected);
    }
}

#include "block_list_widget.moc"
