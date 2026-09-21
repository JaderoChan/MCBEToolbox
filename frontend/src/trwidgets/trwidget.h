#pragma once

#include <qwidget.h>
#include <qevent.h>

#include <easy_translate.hpp>

class TrWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TrWidget(QWidget* parent = nullptr);

    virtual void updateText();

protected:
    void changeEvent(QEvent* event) override;
};
