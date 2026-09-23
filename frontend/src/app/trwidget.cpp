#include "trwidget.h"

TrWidget::TrWidget(QWidget* parent) : QWidget(parent) { updateText(); }

void TrWidget::updateText() {}

void TrWidget::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        updateText();
    QWidget::changeEvent(event);
}

TrMainWindow::TrMainWindow(QWidget* parent) : QMainWindow(parent) { updateText(); }

void TrMainWindow::updateText(){}

void TrMainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        updateText();
    QMainWindow::changeEvent(event);
}

TrDialog::TrDialog(QWidget* parent) : QDialog(parent) { updateText(); }

void TrDialog::updateText() {}

void TrDialog::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        updateText();
    QDialog::changeEvent(event);
}
