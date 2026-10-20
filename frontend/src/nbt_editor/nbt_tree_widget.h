#pragma once

#include <map>

#include <mcnbt/mcnbt.hpp>

#include <components/trwidget.h>
#include "nbt_tree_view.h"

class QToolButton;
class QStackedWidget;
class QStandardItem;
class CodeTextEdit;

class NbtTreeWidget : public TrWidget
{
public:
    explicit NbtTreeWidget(QWidget* parent = nullptr);

    NbtTreeView* nbtView() const { return nbtView_; }

protected:
    void updateText() override;

private:
    void togglePage();
    void updateButtonState();

    QToolButton* renameButton_ = nullptr;
    QToolButton* editButton_   = nullptr;
    QToolButton* removeButton_ = nullptr;
    QToolButton* snbtButton_   = nullptr;

    std::map<nbt::TagType, QToolButton*> addButtons_;

    QStackedWidget* stackedWidget_  = nullptr;
    NbtTreeView*    nbtView_        = nullptr;
    CodeTextEdit*   snbtWidget_     = nullptr;
    bool            needUpdateSnbt_ = true;
};
