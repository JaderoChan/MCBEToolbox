#include "mywidget.h"

MyWidget::MyWidget(QWidget* parent)
    : QWidget(parent)
{
    connect(
        &LanguageManager::instance(),    &LanguageManager::loadedLanguageChanged,
        this, &MyWidget::updateText);
    connect(
        &ColorSchemeManager::instance(), &ColorSchemeManager::loadedColorSchemeChanged,
        this, &MyWidget::updateColor);
}

void MyWidget::updateText(LanguageManager::Language /* loaded */) {}

void MyWidget::updateColor(ColorSchemeManager::ColorScheme /* loaded*/) {}

MyMainWindow::MyMainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    connect(
        &LanguageManager::instance(),    &LanguageManager::loadedLanguageChanged,
        this, &MyMainWindow::updateText);
    connect(
        &ColorSchemeManager::instance(), &ColorSchemeManager::loadedColorSchemeChanged,
        this, &MyMainWindow::updateColor);
}

void MyMainWindow::updateText(LanguageManager::Language /* loaded */) {}

void MyMainWindow::updateColor(ColorSchemeManager::ColorScheme /* loaded*/) {}

MyDialog::MyDialog(QWidget* parent)
    : QDialog(parent)
{
    connect(
        &LanguageManager::instance(),    &LanguageManager::loadedLanguageChanged,
        this, &MyDialog::updateText);
    connect(
        &ColorSchemeManager::instance(), &ColorSchemeManager::loadedColorSchemeChanged,
        this, &MyDialog::updateColor);
}

void MyDialog::updateText(LanguageManager::Language /* loaded */) {}

void MyDialog::updateColor(ColorSchemeManager::ColorScheme /* loaded*/) {}
