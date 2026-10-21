#pragma once

#include <qdialog.h>
#include <qmainwindow.h>
#include <qwidget.h>

#include <easy_translate.hpp>

class MyWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MyWidget(QWidget* parent = nullptr);

public slots:
    virtual void updateText();
    virtual void updateColor();
};

class MyMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MyMainWindow(QWidget* parent = nullptr);

public slots:
    virtual void updateText();
    virtual void updateColor();
};

class MyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MyDialog(QWidget* parent = nullptr);

public slots:
    virtual void updateText();
    virtual void updateColor();
};
