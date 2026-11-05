#pragma once

#include <qobject.h>

/** 用于过滤掉 ComboBox、SPinBox 等控件的滚轮滚动的响应行为（反人类的设计），以防误操作。 */
class NoWheelFocusFilter : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
};

/** 禁用指定控件的滚轮事件。 */
#define QT_BAN_WHEEL_FOCUS(widget) (widget)->installEventFilter(new NoWheelFocusFilter(widget))
