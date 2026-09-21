#include "trwidget.h"

TrWidget::TrWidget(QWidget* parent)
    : QWidget(parent)
{
    updateText();
}

void TrWidget::updateText()
{}

void TrWidget::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        updateText();
    QWidget::changeEvent(event);
}
