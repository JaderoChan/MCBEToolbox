#pragma once

#include <type_traits> // forward

#include <qstring.h>

/** 获取更好的数字文本；保证数字具有两位，使用 `0` 进行左补位。 */
inline QString getPreferredNumberString(int num)
{
    return QStringLiteral("%1").arg(num, 2, 10, QChar('0'));
}

/** 格式化字符串便利函数。 */
template <typename ...Args>
QString formatString(const char* format, Args&& ...args)
{
    QString str(format);
    ((str = str.arg(std::forward<Args>(args))), ...);
    return str;
}
