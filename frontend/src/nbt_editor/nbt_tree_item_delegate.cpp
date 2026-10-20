#include "nbt_tree_item_delegate.h"

#include <qapplication.h>
#include <qfontmetrics.h>
#include <qpainter.h>
#include <qstyle.h>

#include <easy_translate.hpp>

#include "nbt_tree_view.h"

NbtTreeItemDelegate::NbtTreeItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{}

void NbtTreeItemDelegate::paint(
    QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    const auto& isCompoundSummary = index.data(NbtTreeView::IsCompoundSummaryRole);
    const auto& summaryValue      = index.data(NbtTreeView::SummaryValueRole);
    if (!isCompoundSummary.isValid() || !summaryValue.isValid())
        return;

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    const auto* s          = opt.widget ? opt.widget->style() : qApp->style();
    const auto  textRect   = s->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);
    const auto  textMargin = s->pixelMetric(QStyle::PM_FocusFrameHMargin, &opt, opt.widget) + 1;

    const QString summary = QStringLiteral("%1 %2")
        .arg(summaryValue.toULongLong())
        .arg(isCompoundSummary.toBool() ? EASYTR("entries") : EASYTR("elements"));

    const int textWidth = QFontMetrics(opt.font).horizontalAdvance(opt.text);
    QRect summaryRect = textRect;
    summaryRect.setLeft(textRect.left() + textMargin + textWidth);

    QFont summaryFont = opt.font;
    summaryFont.setItalic(true);
    const auto& summaryColor = opt.palette.color(QPalette::Disabled, QPalette::Text);

    painter->save();
    painter->setFont(summaryFont);
    painter->setPen(summaryColor);
    painter->drawText(summaryRect, Qt::AlignVCenter | Qt::AlignLeft, summary);
    painter->restore();
}

QWidget* NbtTreeItemDelegate::createEditor(
    QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    return nullptr;
}
