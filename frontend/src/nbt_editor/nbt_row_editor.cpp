#include "nbt_row_editor.h"

#include <qboxlayout.h>
#include <qevent.h>
#include <qlineedit.h>
#include <qtoolbutton.h>

#include <easy_translate.hpp>

#include <components/color_scheme_manager.h>

NbtRowEditor::NbtRowEditor(QWidget* parent)
    : TrWidget(parent)
{
    constexpr int kEditMinWidth = 80;
    constexpr int kEditMaxWidth = 480;

    nameEdit_      = new QLineEdit(this);
    valueEdit_     = new QLineEdit(this);
    confirmButton_ = new QToolButton(this);
    cancelButton_  = new QToolButton(this);

    for (QLineEdit* edit : {nameEdit_, valueEdit_})
    {
        edit->setMinimumWidth(kEditMinWidth);
        edit->setMaximumWidth(kEditMaxWidth);
        edit->installEventFilter(this);
    }

    for (QToolButton* button : {confirmButton_, cancelButton_})
    {
        button->setAutoRaise(true);
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setFocusPolicy(Qt::NoFocus);
        button->setCursor(Qt::PointingHandCursor);
        button->setIconSize(QSize(16, 16));
        button->setStyleSheet(
            QStringLiteral("QToolButton { border: none; background: transparent; padding: 0px; }")
        );
    }
    connect(confirmButton_, &QToolButton::clicked, this, &NbtRowEditor::confirmed);
    connect(cancelButton_,  &QToolButton::clicked, this, &NbtRowEditor::cancelled);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 0, 2, 0);
    layout->setSpacing(4);
    layout->addWidget(nameEdit_);
    layout->addWidget(valueEdit_);
    layout->addWidget(confirmButton_);
    layout->addWidget(cancelButton_);
    layout->addStretch(0);

    updateIcons();
    updateText();
    connect(
        &ColorSchemeManager::getInstance(), &ColorSchemeManager::loadedColorSchemeChanged,
        this, &NbtRowEditor::updateIcons
    );
}

void NbtRowEditor::setNameEditable(bool editable)
{
    nameEdit_->setVisible(editable);
}

void NbtRowEditor::setValueEditable(bool editable)
{
    valueEdit_->setVisible(editable);
}

bool NbtRowEditor::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        const auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape)
        {
            emit cancelled();
            return true;
        }
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
        {
            emit confirmed();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void NbtRowEditor::updateText()
{
    confirmButton_->setToolTip(EASYTR("Confirm"));
    cancelButton_->setToolTip(EASYTR("Cancel"));
}

void NbtRowEditor::updateIcons()
{
    confirmButton_->setIcon(QIcon(":/icons/nbt_editor/checkmark.png"));
    cancelButton_->setIcon(QIcon(":/icons/nbt_editor/cross.png"));
}
