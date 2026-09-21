#pragma once

#include <qdialog.h>
#include <qevent.h>

#include <easy_translate.hpp>

class TrDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TrDialog(QWidget* parent = nullptr);

    virtual void updateText();

protected:
    void changeEvent(QEvent* event) override;
};
