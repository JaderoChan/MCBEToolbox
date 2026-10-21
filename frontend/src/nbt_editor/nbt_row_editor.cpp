#include "nbt_row_editor.h"

#include <limits>

#include <qapplication.h>
#include <qevent.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qregularexpression.h>
#include <qtoolbutton.h>
#include <qvalidator.h>

#include <easy_translate.hpp>

#include "nbt_utils.h"

namespace
{

// QIntValidator 不支持 long long 型范围。
class IntegerValidator : public QValidator
{
public:
    IntegerValidator(qlonglong min, qlonglong max, QObject* parent)
        : QValidator(parent), min_(min), max_(max)
    {}

    State validate(QString& input, int&) const override
    {
        const auto& trimmed = input.trimmed();
        if (trimmed.isEmpty() || trimmed == QStringLiteral("-"))
            return Intermediate;

        static const QRegularExpression digitsOnly(QStringLiteral("^-?\\d+$"));
        if (!digitsOnly.match(trimmed).hasMatch())
            return Invalid;

        bool ok = false;
        const auto& value = trimmed.toLongLong(&ok);
        if (!ok || value < min_ || value > max_)
            return Invalid;
        return Acceptable;
    }

private:
    qlonglong min_ = 0;
    qlonglong max_ = 0;
};

// 构建与指定类型对应的数值验证器。
QValidator* tagTypeValidator(nbt::TagType type, QObject* parent = nullptr)
{
    switch (type)
    {
        case nbt::TT_BYTE:
            return new IntegerValidator(
                std::numeric_limits<int8_t>::min(),
                std::numeric_limits<int8_t>::max(),
                parent
            );
        case nbt::TT_SHORT:
            return new IntegerValidator(
                std::numeric_limits<int16_t>::min(),
                std::numeric_limits<int16_t>::max(),
                parent
            );
        case nbt::TT_INT:
            return new IntegerValidator(
                std::numeric_limits<int32_t>::min(),
                std::numeric_limits<int32_t>::max(),
                parent
            );
        case nbt::TT_LONG:
            return new IntegerValidator(
                std::numeric_limits<int64_t>::min(),
                std::numeric_limits<int64_t>::max(),
                parent
            );
        case nbt::TT_FLOAT: // Fallthrough
        case nbt::TT_DOUBLE:
            return new QDoubleValidator(parent);
        default:
            return nullptr;
    }
}

} // namespace

NbtRowEditor::NbtRowEditor(nbt::TagType type, bool isNamingTag, QWidget* parent)
    : type_(type), isNamingTag_(isNamingTag), MyWidget(parent)
{
    constexpr int EDIT_MIN_WIDTH = 120;
    constexpr int EDIT_MAX_WIDTH = 600;

    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Base);

    if (!editable()) return;

    keyEdit_       = new QLineEdit(this);
    valueEdit_     = new QLineEdit(this);
    confirmButton_ = new QToolButton(this);
    cancelButton_  = new QToolButton(this);

    for (auto* edit : {keyEdit_, valueEdit_})
    {
        edit->setMinimumWidth(EDIT_MIN_WIDTH);
        edit->setMaximumWidth(EDIT_MAX_WIDTH);
        edit->installEventFilter(this);
    }

    for (auto* button : {confirmButton_, cancelButton_})
    {
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setIconSize(QSize(16, 16));
        button->setStyleSheet(QStringLiteral("border: none; background: transparent; padding: 0px;"));
        button->setFocusPolicy(Qt::ClickFocus);
    }

    confirmButton_->setIcon(QIcon(":/icons/nbt_editor/checkmark.png"));
    cancelButton_->setIcon(QIcon(":/icons/nbt_editor/cross.png"));

    connect(confirmButton_, &QToolButton::clicked, this, [this]() { emit confirmed(); });
    connect(cancelButton_,  &QToolButton::clicked, this, [this]() { emit cancelled(); });

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 0, 2, 0);
    layout->setSpacing(4);
    layout->addWidget(keyEdit_);
    layout->addWidget(valueEdit_);
    layout->addWidget(confirmButton_);
    layout->addWidget(cancelButton_);
    layout->addStretch();

    if (!keyEditable())   keyEdit_->hide();
    if (!valueEditable()) valueEdit_->hide();

    validator_ = tagTypeValidator(type_, this);
    if (validator_) valueEdit_->setValidator(validator_);

    updateText();
}

nbt::TagType NbtRowEditor::type() const
{
    return type_;
}

bool NbtRowEditor::isNamingTag() const
{
    return isNamingTag_;
}

bool NbtRowEditor::keyEditable() const
{
    return nbt_utils::tagKeyEditable(type_, isNamingTag_);
}

bool NbtRowEditor::valueEditable() const
{
    return nbt_utils::tagValueEditable(type_, isNamingTag_);
}

bool NbtRowEditor::editable() const
{
    return nbt_utils::tagEditable(type_, isNamingTag_);
}

void NbtRowEditor::setKeyText(const QString& text)
{
    if (keyEditable())
        keyEdit_->setText(text);
}

void NbtRowEditor::setValueText(const QString& text)
{
    if (!valueEditable())
        return;

    QString s = text;
    int pos;
    if (!validator_ || validator_->validate(s, pos) == QValidator::Acceptable)
        valueEdit_->setText(text);
}

QString NbtRowEditor::keyText() const
{
    return keyEdit_->text();
}

QString NbtRowEditor::valueText() const
{
    return valueEdit_->text();
}

void NbtRowEditor::focusKeyEdit()
{
    if (keyEditable())
        keyEdit_->setFocus(Qt::MouseFocusReason);
}

void NbtRowEditor::focusValueEdit()
{
    if (valueEditable())
        valueEdit_->setFocus(Qt::MouseFocusReason);
}

void NbtRowEditor::selectKeyEditAll()
{
    if (keyEditable())
        keyEdit_->selectAll();
}

void NbtRowEditor::selectValueEditAll()
{
    if (valueEditable())
        valueEdit_->selectAll();
}

bool NbtRowEditor::eventFilter(QObject* obj, QEvent* e)
{
    if (e->type() == QEvent::KeyPress)
    {
        const auto* keyEvent = static_cast<QKeyEvent*>(e);

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
    else if (e->type() == QEvent::FocusOut)
    {
        auto* focusWidget = QApplication::focusWidget();
        if (focusWidget != this && !isAncestorOf(focusWidget))
            emit focusLost();
    }

    return QWidget::eventFilter(obj, e);
}

void NbtRowEditor::updateText()
{
    confirmButton_->setToolTip(EASYTR("Confirm"));
    cancelButton_->setToolTip(EASYTR("Cancel"));
}
