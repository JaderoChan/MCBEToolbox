#include "adaptive_widget.h"

AdaptiveWidget::AdaptiveWidget(QWidget* parent)
    : QWidget(parent)
{
    connect(
        &LanguageManager::instance(), &LanguageManager::loadedLanguageChanged,
        this, &AdaptiveWidget::updateText
    );
    connect(
        &ColorSchemeManager::instance(), &ColorSchemeManager::loadedColorSchemeChanged,
        this, &AdaptiveWidget::updateColor
    );
}

void AdaptiveWidget::updateText(LanguageManager::Language /* loaded */) {}

void AdaptiveWidget::updateColor(ColorSchemeManager::ColorScheme /* loaded */) {}

AdaptiveMainWindow::AdaptiveMainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    connect(
        &LanguageManager::instance(), &LanguageManager::loadedLanguageChanged,
        this, &AdaptiveMainWindow::updateText
    );
    connect(
        &ColorSchemeManager::instance(), &ColorSchemeManager::loadedColorSchemeChanged,
        this, &AdaptiveMainWindow::updateColor
    );
}

void AdaptiveMainWindow::updateText(LanguageManager::Language /* loaded */) {}

void AdaptiveMainWindow::updateColor(ColorSchemeManager::ColorScheme /* loaded */) {}

AdaptiveDialog::AdaptiveDialog(QWidget* parent)
    : QDialog(parent)
{
    connect(
        &LanguageManager::instance(), &LanguageManager::loadedLanguageChanged,
        this, &AdaptiveDialog::updateText
    );
    connect(
        &ColorSchemeManager::instance(), &ColorSchemeManager::loadedColorSchemeChanged,
        this, &AdaptiveDialog::updateColor
    );
}

void AdaptiveDialog::updateText(LanguageManager::Language /* loaded */) {}

void AdaptiveDialog::updateColor(ColorSchemeManager::ColorScheme /* loaded */) {}
