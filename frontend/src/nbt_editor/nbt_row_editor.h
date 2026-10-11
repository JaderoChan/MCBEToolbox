#pragma once

#include <components/trwidget.h>

class QLineEdit;
class QToolButton;

class NbtRowEditor : public TrWidget
{
    Q_OBJECT

public:
    explicit NbtRowEditor(QWidget* parent = nullptr);

    QLineEdit* nameEdit()  const { return nameEdit_;  }
    QLineEdit* valueEdit() const { return valueEdit_; }

    void setNameEditable(bool editable);
    void setValueEditable(bool editable);

signals:
    void confirmed();
    void cancelled();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void updateText() override;

private:
    void updateIcons();

    QLineEdit*   nameEdit_      = nullptr;
    QLineEdit*   valueEdit_     = nullptr;
    QToolButton* confirmButton_ = nullptr;
    QToolButton* cancelButton_  = nullptr;
};
