#include "flow_grid_layout.h"

FlowGridLayout::FlowGridLayout(int margin, int spacing, int minItemSize, int maxItemSize, QWidget* parent)
    : QLayout(parent), minItemSize_(minItemSize), maxItemSize_(maxItemSize)
{
    setContentsMargins(margin, margin, margin, margin);
    setSpacing(spacing);
}

FlowGridLayout::~FlowGridLayout()
{
    QLayoutItem* item = nullptr;
    while ((item = takeAt(0)))
        delete item;
}

void FlowGridLayout::invalidate()
{
    cacheDirty_ = true;
    QLayout::invalidate();
}

void FlowGridLayout::addItem(QLayoutItem* item)
{
    items_.append(item);
}

int FlowGridLayout::count() const
{
    return items_.size();
}

QLayoutItem* FlowGridLayout::itemAt(int index) const
{
    return (index >= 0 && index < static_cast<int>(items_.size())) ? items_[index] : nullptr;
}

QLayoutItem* FlowGridLayout::takeAt(int index)
{
    QLayoutItem* ret = (index >= 0 && index < static_cast<int>(items_.size())) ? items_.takeAt(index) : nullptr;
    invalidate();
    return ret;
}

Qt::Orientations FlowGridLayout::expandingDirections() const
{
    return {};
}

bool FlowGridLayout::hasHeightForWidth() const
{
    return true;
}

int FlowGridLayout::heightForWidth(int width) const
{
    if (cacheDirty_ || width != cachedWidth_)
    {
        updateCached(width);
        cachedWidth_ = width;
        cacheDirty_ = false;
    }

    const QMargins margins = contentsMargins();
    const int h = cachedRows_ * cachedItemSize_ + qMax(0, cachedRows_ - 1) * spacing();
    return h + margins.top() + margins.bottom();
}

QSize FlowGridLayout::minimumSize() const
{
    const QMargins margins = contentsMargins();
    return QSize(
        minItemSize_ + margins.left() + margins.right(),
        minItemSize_ + margins.top()  + margins.bottom()
    );
}

QSize FlowGridLayout::sizeHint() const
{
    return minimumSize();
}

void FlowGridLayout::setGeometry(const QRect& rect)
{
    QLayout::setGeometry(rect);

    const int w = rect.width();
    if (cacheDirty_ || w != cachedWidth_)
    {
        updateCached(w);
        cachedWidth_ = w;
        cacheDirty_ = false;
    }

    const QMargins margins = contentsMargins();
    const QRect r = rect.adjusted(margins.left(), margins.top(), -margins.right(), -margins.bottom());
    int index = 0;
    for (int row = 0; row < cachedRows_; ++row)
    {
        const int y = r.y() + row * (cachedItemSize_ + spacing());
        for (int col = 0; col < cachedCols_ && index < items_.size(); ++col, ++index)
        {
            const int x = r.x() + col * (cachedItemSize_ + spacing());
            items_[index]->setGeometry(QRect(x, y, cachedItemSize_, cachedItemSize_));
        }
    }
}

void FlowGridLayout::updateCached(int width) const
{
    const QMargins margins = contentsMargins();
    const int w = qMax(1, width - margins.left() - margins.right());

    if (items_.empty())
    {
        cachedItemSize_ = 0;
        cachedRows_     = 0;
        cachedCols_     = 0;
        return;
    }

    int cols = qMax(1, (w + spacing()) / (minItemSize_ + spacing()));
    int itemSize = (w - (cols - 1) * spacing()) / cols;
    if (itemSize > maxItemSize_)
    {
        itemSize = maxItemSize_;
        cols = qMax(1, (w + spacing()) / (itemSize + spacing()));
    }
    itemSize = qMax(minItemSize_, qMin(itemSize, maxItemSize_));

    const int rows = (items_.size() + cols - 1) / cols;

    cachedItemSize_ = itemSize;
    cachedRows_     = rows;
    cachedCols_     = cols;
}
