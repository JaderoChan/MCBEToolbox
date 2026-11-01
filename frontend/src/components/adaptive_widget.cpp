#include "adaptive_widget.h"

AdaptiveWidget::AdaptiveWidget(QWidget* parent)
    : QWidget(parent)
{
    connect(
        &LanguageManager::instance(), &LanguageManager::loadedLanguageChanged,
        this, &AdaptiveWidget::refreshText
    );
    connect(
        &ColorSchemeManager::instance(), &ColorSchemeManager::loadedColorSchemeChanged,
        this, &AdaptiveWidget::refreshColor
    );
}

void AdaptiveWidget::refreshText() {}

void AdaptiveWidget::refreshColor() {}

AdaptiveMainWindow::AdaptiveMainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    connect(
        &LanguageManager::instance(), &LanguageManager::loadedLanguageChanged,
        this, &AdaptiveMainWindow::refreshText
    );
    connect(
        &ColorSchemeManager::instance(), &ColorSchemeManager::loadedColorSchemeChanged,
        this, &AdaptiveMainWindow::refreshColor
    );
}

void AdaptiveMainWindow::refreshText() {}

void AdaptiveMainWindow::refreshColor() {}

AdaptiveDialog::AdaptiveDialog(QWidget* parent)
    : QDialog(parent)
{
    connect(
        &LanguageManager::instance(), &LanguageManager::loadedLanguageChanged,
        this, &AdaptiveDialog::refreshText
    );
    connect(
        &ColorSchemeManager::instance(), &ColorSchemeManager::loadedColorSchemeChanged,
        this, &AdaptiveDialog::refreshColor
    );
}

void AdaptiveDialog::refreshText() {}

void AdaptiveDialog::refreshColor() {}
