#pragma once

#include <qdialog.h>
#include <qevent.h>
#include <qmainwindow.h>
#include <qwidget.h>

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

class TrMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit TrMainWindow(QWidget* parent = nullptr);
    virtual void updateText();

protected:
    void changeEvent(QEvent* event) override;
};

class TrDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TrDialog(QWidget* parent = nullptr);
    virtual void updateText();

protected:
    void changeEvent(QEvent* event) override;
};
