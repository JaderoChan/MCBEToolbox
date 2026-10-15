#include "trwidget.h"

TrWidget::TrWidget(QWidget* parent) : QWidget(parent) { updateText(); }

void TrWidget::updateText() {}

void TrWidget::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange)
        updateText();
}

TrMainWindow::TrMainWindow(QWidget* parent) : QMainWindow(parent) { updateText(); }

void TrMainWindow::updateText(){}

void TrMainWindow::changeEvent(QEvent* e)
{
    QMainWindow::changeEvent(e);
    if (e->type() == QEvent::LanguageChange)
        updateText();
}

TrDialog::TrDialog(QWidget* parent) : QDialog(parent) { updateText(); }

void TrDialog::updateText() {}

void TrDialog::changeEvent(QEvent* e)
{
    QDialog::changeEvent(e);
    if (e->type() == QEvent::LanguageChange)
        updateText();
}
