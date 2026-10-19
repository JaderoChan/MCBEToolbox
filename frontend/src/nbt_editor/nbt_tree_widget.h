#pragma once

#include <map>

#include <mcnbt/mcnbt.hpp>

#include <components/trwidget.h>
#include "nbt_tree_view.h"

class QToolButton;
class QStandardItem;

class NbtTreeWidget : public TrWidget
{
public:
    explicit NbtTreeWidget(QWidget* parent = nullptr);

    const NbtTreeView* nbtTreeView() const { return view_; }
    NbtTreeView*       nbtTreeView()       { return view_; }

protected:
    void updateText() override;

private:
    void updateButtonState(const QStandardItem* item);

    QToolButton* renameButton_;
    QToolButton* editButton_;
    QToolButton* removeButton_;

    std::map<nbt::TagType, QToolButton*> addButtons_;

    NbtTreeView* view_;
};
