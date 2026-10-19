#include "nbt_tree_widget.h"

#include <qframe.h>
#include <qlayout.h>
#include <qstandarditemmodel.h>
#include <qtoolbutton.h>

#include <easy_translate.hpp>

#include "nbt_utils.h"

NbtTreeWidget::NbtTreeWidget(QWidget* parent)
    : TrWidget(parent)
{
    // 构建顶部工具栏 UI。
    QWidget*     toolbar       = new QWidget(this);
    QHBoxLayout* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(2);

    // 基础 Tag 操作按钮。
    renameButton_ = new QToolButton(this);
    editButton_   = new QToolButton(this);
    removeButton_ = new QToolButton(this);
    for (auto& button : {renameButton_, editButton_, removeButton_})
    {
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        button->setFocusPolicy(Qt::ClickFocus);
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        button->setFixedHeight(24);

        toolbarLayout->addWidget(button);
    }
    connect(
        removeButton_, &QToolButton::clicked,
        this, [this]() { view_->removeItem(view_->selectedItem());          }
    );
    connect(
        renameButton_, &QToolButton::clicked,
        this, [this]() { view_->invokeEditItemKey(view_->selectedItem());   }
    );
    connect(
        editButton_,   &QToolButton::clicked,
        this, [this]() { view_->invokeEditItemValue(view_->selectedItem()); }
    );

    // 分割线。
    QFrame* separator = new QFrame(this);
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    toolbarLayout->addSpacing(2);
    toolbarLayout->addWidget(separator);
    toolbarLayout->addSpacing(2);

    // Tag 增添按钮。
    for (nbt::TagType type : nbt_utils::addableTagTypes())
    {
        QToolButton* button = new QToolButton(this);
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        button->setFocusPolicy(Qt::ClickFocus);
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setIcon(nbt_utils::tagTypeIcon(type));
        button->setIconSize(QSize(24, 24));
        button->setStyleSheet(QStringLiteral("border: none; background: transparent; padding: 0px;"));

        toolbarLayout->addWidget(button);
        addButtons_[type] = button;

        connect(
            button, &QToolButton::clicked,
            this, [this, type]() { view_->invokeAddItem(view_->selectedItem(), type); }
        );
    }

    toolbarLayout->addStretch();

    // 主视图
    view_ = new NbtTreeView(this);

    // 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(2);
    mainLayout->addWidget(toolbar);
    mainLayout->addWidget(view_);

    connect(view_, &NbtTreeView::selectedItemChanged, this, &NbtTreeWidget::updateButtonState);

    updateButtonState(nullptr);
    updateText();
}

void NbtTreeWidget::updateText()
{
    renameButton_->setText(EASYTR("Rename"));
    editButton_->setText(EASYTR("Edit"));
    removeButton_->setText(EASYTR("Remove"));
    addButtons_[nbt::TT_BYTE]->setToolTip(EASYTR("Add Byte Tag"));
    addButtons_[nbt::TT_SHORT]->setToolTip(EASYTR("Add Short Tag"));
    addButtons_[nbt::TT_INT]->setToolTip(EASYTR("Add Int Tag"));
    addButtons_[nbt::TT_LONG]->setToolTip(EASYTR("Add Long Tag"));
    addButtons_[nbt::TT_FLOAT]->setToolTip(EASYTR("Add Float Tag"));
    addButtons_[nbt::TT_DOUBLE]->setToolTip(EASYTR("Add Double Tag"));
    addButtons_[nbt::TT_STRING]->setToolTip(EASYTR("Add String Tag"));
    addButtons_[nbt::TT_BYTE_ARRAY]->setToolTip(EASYTR("Add Byte Array Tag"));
    addButtons_[nbt::TT_INT_ARRAY]->setToolTip(EASYTR("Add Int Array Tag"));
    addButtons_[nbt::TT_LONG_ARRAY]->setToolTip(EASYTR("Add Long Array Tag"));
    addButtons_[nbt::TT_LIST]->setToolTip(EASYTR("Add List Tag"));
    addButtons_[nbt::TT_COMPOUND]->setToolTip(EASYTR("Add Compound Tag"));
}

void NbtTreeWidget::updateButtonState(const QStandardItem* item)
{
    // 先全部禁用。
    for (auto& button : {renameButton_, editButton_, removeButton_})
        button->setDisabled(true);
    for (auto& [type, button] : addButtons_)
        button->setDisabled(true);

    // 如果没有选中项直接返回。
    if (!item)
        return;

    if (view_->isArrayElement(item))
    {
        // 数组子元素只启用数值编辑与删除按钮。
        editButton_->setEnabled(true);
        removeButton_->setEnabled(true);
    }
    else
    {
        // 先获取 item 对应的 Tag。
        const auto* tag = const_cast<const NbtTreeView*>(view_)->itemTag(item);
        if (!tag)
            return;

        // 根据 Tag 类型决定是否启用重命名与数值编辑按钮。
        const nbt::TagType type = tag->type();
        const bool isNamingTag  = view_->isNamingTag(item);
        if (nbt_utils::tagKeyEditable(type, isNamingTag))   renameButton_->setEnabled(true);
        if (nbt_utils::tagValueEditable(type, isNamingTag)) editButton_->setEnabled(true);
        // 如果当前 item 不是根项则启用删除按钮。
        if (item != view_->rootItem())                      removeButton_->setEnabled(true);

        // 根据当前 item 对应的 Tag 启用不同增添按钮。
        for (nbt::TagType type : nbt_utils::addableTagTypes(tag))
            addButtons_[type]->setEnabled(true);
    }
}
