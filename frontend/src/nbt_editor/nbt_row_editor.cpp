#include "nbt_row_editor.h"

#include <limits>

#include <qevent.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qregularexpression.h>
#include <qtoolbutton.h>
#include <qvalidator.h>

#include <easy_translate.hpp>

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
        const QString trimmed = input.trimmed();
        if (trimmed.isEmpty() || trimmed == QStringLiteral("-"))
            return Intermediate;

        static const QRegularExpression digitsOnly(QStringLiteral("^-?\\d+$"));
        if (!digitsOnly.match(trimmed).hasMatch())
            return Invalid;

        bool ok = false;
        const qlonglong value = trimmed.toLongLong(&ok);
        if (!ok || value < min_ || value > max_)
            return Invalid;
        return Acceptable;
    }

private:
    qlonglong min_ = 0;
    qlonglong max_ = 0;
};

QValidator* validatorForTagType(nbt::TagType type, QObject* parent)
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
    : type_(type), isNamingTag_(isNamingTag), TrWidget(parent)
{
    constexpr int EDIT_MIN_WIDTH = 80;
    constexpr int EDIT_MAX_WIDTH = 320;

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
        button->setStyleSheet("border: none; background: transparent; padding: 0px;");
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
    layout->addStretch(0);

    if (!keyEditable())   keyEdit_->hide();
    if (!valueEditable()) valueEdit_->hide();

    validator_ = validatorForTagType(type_, this);
    if (validator_) valueEdit_->setValidator(validator_);

    updateText();
}

bool NbtRowEditor::keyEditable(nbt::TagType type, bool isNamingTag)
{
    return isNamingTag && type != nbt::TT_END;
}

bool NbtRowEditor::valueEditable(nbt::TagType type, bool isNamingTag)
{
    (void)isNamingTag;
    return
        type == nbt::TT_BYTE || type == nbt::TT_SHORT || type == nbt::TT_INT    ||
        type == nbt::TT_LONG || type == nbt::TT_FLOAT || type == nbt::TT_DOUBLE ||
        type == nbt::TT_STRING;
}

bool NbtRowEditor::editable(nbt::TagType type, bool isNamingTag)
{
    return keyEditable(type, isNamingTag) || valueEditable(type, isNamingTag);
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
    return keyEditable(type_, isNamingTag_);
}

bool NbtRowEditor::valueEditable() const
{
    return valueEditable(type_, isNamingTag_);
}

bool NbtRowEditor::editable() const
{
    return editable(type_, isNamingTag_);
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

    return QWidget::eventFilter(obj, e);
}

void NbtRowEditor::updateText()
{
    confirmButton_->setToolTip(EASYTR("Confirm"));
    cancelButton_->setToolTip(EASYTR("Cancel"));
}
