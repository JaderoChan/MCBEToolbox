#include "trmainwindow.h"

TrMainWindow::TrMainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    updateText();
}

void TrMainWindow::updateText()
{}

void TrMainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
        updateText();
    QMainWindow::changeEvent(event);
}
