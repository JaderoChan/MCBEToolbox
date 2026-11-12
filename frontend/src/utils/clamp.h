#pragma once

#include <assert.h>

#include <qsize.h>

template <typename T>
inline T clamp(const T& value, const T& min, const T& max)
{
    assert(min <= max);
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

inline QSize clampSize(const QSize& value, const QSize& min, const QSize& max)
{
    return QSize(
        clamp(value.width(),  min.width(),  max.width()),
        clamp(value.height(), min.height(), max.height())
    );
}
