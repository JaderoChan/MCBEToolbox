#pragma once

#include <qstyleditemdelegate.h>

/** 为 #NbtTreeView 节点绘制外观的自定义委托。 */
class NbtTreeItemDelegate : public QStyledItemDelegate
{
public:
    explicit NbtTreeItemDelegate(QObject* parent = nullptr);

    void paint(
        QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QWidget* createEditor(
        QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};
