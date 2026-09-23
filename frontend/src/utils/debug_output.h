#pragma once

#include <type_traits> // forward

#include <qdebug.h>

#include "format_string.h"

template <typename ...Args>
void debugOut(QDebug io, const char* format, Args&& ...args)
{
    io.noquote() << formatString(format, std::forward<Args>(args)...);
}
