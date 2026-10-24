#pragma once

#include <type_traits> // forward

#include <qdebug.h>

#include "format_string.h"

/** 用于向 QDebug 对象输出格式化文本的便利函数。 */
template <typename ...Args>
void debugOut(QDebug io, const char* format, Args&& ...args)
{
    io.noquote() << formatString(format, std::forward<Args>(args)...);
}
