#include "no_wheel_focus_filter.h"

#include <qcoreapplication.h>
#include <qevent.h>
#include <qwidget.h>

bool NoWheelFocusFilter::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() != QEvent::Wheel)
        return false;

    auto* widget = qobject_cast<QWidget*>(watched);
    if (!widget)
        return false;

    event->ignore();
    // 转发给父控件。
    if (auto* parent = widget->parentWidget())
        QCoreApplication::sendEvent(parent, event);
    return true;
}
