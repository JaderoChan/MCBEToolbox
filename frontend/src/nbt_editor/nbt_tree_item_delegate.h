#pragma once

#include <qstyleditemdelegate.h>

class NbtTreeView;

class NbtTreeItemDelegate : public QStyledItemDelegate
{
public:
    explicit NbtTreeItemDelegate(NbtTreeView* view);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    // 供 NbtTreeView::editTargetForClick() 判断双击位置命中的是键名区域还是键值区域。
    int keyBoundaryX(const QStyleOptionViewItem& option, const QModelIndex& index) const;
    // 供 NbtTreeView::edit() 判断双击位置是否落在文字区域左侧。
    int textLeftX(const QStyleOptionViewItem& option, const QModelIndex& index) const;

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    // 键名区域与键值区域的绘制/命中测算结果，供 paint() 与 keyBoundaryX()/textLeftX() 共用。
    struct SplitLayout
    {
        QRect   textRect;
        QString keyPart;
        QString valuePart;
        int     boundaryX = 0;
    };

    SplitLayout computeSplit(const QStyleOptionViewItem& opt, const QModelIndex& index) const;

    NbtTreeView* view_ = nullptr;
};
