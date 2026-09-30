#include "trwidget.h"

TrWidget::TrWidget(QWidget* parent) : QWidget(parent) { updateText(); }

void TrWidget::updateText() {}

void TrWidget::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LanguageChange)
        updateText();
}

TrMainWindow::TrMainWindow(QWidget* parent) : QMainWindow(parent) { updateText(); }

void TrMainWindow::updateText(){}

void TrMainWindow::changeEvent(QEvent* event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::LanguageChange)
        updateText();
}

TrDialog::TrDialog(QWidget* parent) : QDialog(parent) { updateText(); }

void TrDialog::updateText() {}

void TrDialog::changeEvent(QEvent* event)
{
    QDialog::changeEvent(event);
    if (event->type() == QEvent::LanguageChange)
        updateText();
}
