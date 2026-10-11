#include "nbt_tree_item_delegate.h"

#include <qfontmetrics.h>
#include <qpainter.h>
#include <qstyle.h>

#include "nbt_tree_view.h"

NbtTreeItemDelegate::NbtTreeItemDelegate(NbtTreeView* view)
    : QStyledItemDelegate(view), view_(view)
{}

void NbtTreeItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    const SplitLayout layout = computeSplit(opt, index);

    opt.text.clear();
    QStyle* s = opt.widget ? opt.widget->style() : view_->style();
    s->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

    const bool summaryStyle = index.data(NbtTreeView::SummaryStyleRole).toBool();
    const QColor normalColor = opt.palette.color(
        (opt.state & QStyle::State_Selected) ? QPalette::HighlightedText : QPalette::Text);

    painter->save();
    painter->setFont(opt.font);
    painter->setPen(normalColor);
    painter->drawText(layout.textRect, Qt::AlignVCenter | Qt::AlignLeft, layout.keyPart);

    if (!layout.valuePart.isEmpty())
    {
        QRect valueRect = layout.textRect;
        valueRect.setLeft(layout.boundaryX);

        QFont valueFont = opt.font;
        QColor valueColor = normalColor;
        if (summaryStyle)
        {
            valueFont.setItalic(true);
            valueColor = opt.palette.color(QPalette::Disabled, QPalette::Text);
        }
        painter->setFont(valueFont);
        painter->setPen(valueColor);
        painter->drawText(valueRect, Qt::AlignVCenter | Qt::AlignLeft, layout.valuePart);
    }
    painter->restore();
}

int NbtTreeItemDelegate::keyBoundaryX(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    return computeSplit(opt, index).boundaryX;
}

int NbtTreeItemDelegate::textLeftX(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    return computeSplit(opt, index).textRect.left();
}

QWidget* NbtTreeItemDelegate::createEditor(QWidget*, const QStyleOptionViewItem&, const QModelIndex&) const
{
    return nullptr;
}

NbtTreeItemDelegate::SplitLayout NbtTreeItemDelegate::computeSplit(
    const QStyleOptionViewItem& opt, const QModelIndex& index) const
{
    SplitLayout layout;
    QStyle* s = opt.widget ? opt.widget->style() : view_->style();
    layout.textRect = s->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);
    layout.textRect.adjust(2, 0, -2, 0);

    const int valueStart = index.data(NbtTreeView::ValueStartRole).toInt();
    layout.keyPart   = valueStart >= 0 ? opt.text.left(valueStart) : opt.text;
    layout.valuePart = valueStart >= 0 ? opt.text.mid(valueStart)  : QString();
    layout.boundaryX = layout.textRect.left() + QFontMetrics(opt.font).horizontalAdvance(layout.keyPart);
    return layout;
}
