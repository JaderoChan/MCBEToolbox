#pragma once

#include <qstring.h>

#include <mcnbt/mcnbt.hpp>

#include <components/mywidget.h>

class QLineEdit;
class QToolButton;
class QValidator;

/** #NbtTreeView 的行内编辑控件，提供键/值的文本编辑与确认/取消功能。 */
class NbtRowEditor : public MyWidget
{
    Q_OBJECT

public:
    explicit NbtRowEditor(nbt::TagType type, bool isNamingTag, QWidget* parent = nullptr);

    nbt::TagType type()  const;
    bool isNamingTag()   const;
    bool keyEditable()   const;
    bool valueEditable() const;
    bool editable()      const;

    void setKeyText(const QString& text);
    void setValueText(const QString& text);

    QString keyText()   const;
    QString valueText() const;

    void focusKeyEdit();
    void focusValueEdit();
    void selectKeyEditAll();
    void selectValueEditAll();

signals:
    void confirmed();
    void cancelled();
    void focusLost();

protected:
    bool eventFilter(QObject* obj, QEvent* e) override;
    void updateText(LanguageManager::Language loaded = LanguageManager::instance().loadedLanguage()) override;

private:
    nbt::TagType type_;
    bool         isNamingTag_;
    QValidator*  validator_     = nullptr;
    QLineEdit*   keyEdit_       = nullptr;
    QLineEdit*   valueEdit_     = nullptr;
    QToolButton* confirmButton_ = nullptr;
    QToolButton* cancelButton_  = nullptr;
};
