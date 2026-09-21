#include "trdialog.h"

TrDialog::TrDialog(QWidget* parent)
    : QDialog(parent)
{
    updateText();
}

void TrDialog::updateText()
{}

void TrDialog::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        updateText();
    QDialog::changeEvent(event);
}
