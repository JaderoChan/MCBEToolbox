#include "mywidget.h"

#include "color_scheme_manager.h"
#include "language_manager.h"

MyWidget::MyWidget(QWidget* parent)
    : QWidget(parent)
{
    connect(
        &LanguageManager::getInstance(),    &LanguageManager::loadedLanguageChanged,
        this, &MyWidget::updateText);
    connect(
        &ColorSchemeManager::getInstance(), &ColorSchemeManager::loadedColorSchemeChanged,
        this, &MyWidget::updateColor);
}

void MyWidget::updateText() {}

void MyWidget::updateColor() {}

MyMainWindow::MyMainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    connect(
        &LanguageManager::getInstance(),    &LanguageManager::loadedLanguageChanged,
        this, &MyMainWindow::updateText);
    connect(
        &ColorSchemeManager::getInstance(), &ColorSchemeManager::loadedColorSchemeChanged,
        this, &MyMainWindow::updateColor);
}

void MyMainWindow::updateText() {}

void MyMainWindow::updateColor() {}

MyDialog::MyDialog(QWidget* parent)
    : QDialog(parent)
{
    connect(
        &LanguageManager::getInstance(),    &LanguageManager::loadedLanguageChanged,
        this, &MyDialog::updateText);
    connect(
        &ColorSchemeManager::getInstance(), &ColorSchemeManager::loadedColorSchemeChanged,
        this, &MyDialog::updateColor);
}

void MyDialog::updateText() {}

void MyDialog::updateColor() {}
