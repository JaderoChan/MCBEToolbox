#pragma once

#include <qmainwindow.h>
#include <qevent.h>

#include <easy_translate.hpp>

class TrMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit TrMainWindow(QWidget* parent = nullptr);

    virtual void updateText();

protected:
    void changeEvent(QEvent* event) override;
};
