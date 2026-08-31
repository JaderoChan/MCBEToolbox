#pragma once

#include <string>
#include <string_view>

struct Rgb
{
    unsigned char r, g, b;
};

inline bool operator==(const Rgb& lhs, const Rgb& rhs)
{
    return (lhs.r == rhs.r) && (lhs.g == rhs.g) && (lhs.b == rhs.b);
}

/** @throw invalid_argument */
Rgb hexToRgb(std::string_view hex);

std::string rgbToHex(const Rgb& rgb, bool bigCase = true, bool withNumberSign = true);
