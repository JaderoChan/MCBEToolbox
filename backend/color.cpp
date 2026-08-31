#include "color.hpp"

#include <stdexcept>

static inline int hexCharToInt(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    throw std::invalid_argument("invalid hex character '" + std::string(1, c) + "'");
}

static inline char intToHexChar(int i, bool bigCase = true)
{
    if (i >= 0  && i <= 9)  return i + '0';
    if (i >= 10 && i <= 15) return (bigCase ? (i - 10 + 'A') : (i - 10 + 'a'));
    throw std::invalid_argument("invalid integer '" + std::to_string(i) + "' for hex char");
}

Rgb hexToRgb(std::string_view hex)
{
    if (hex.empty())
        throw std::invalid_argument("invalid hex RGB color string '" + std::string(hex) + "'");
    if (hex[0] == '#')
        hex = hex.substr(1);

    Rgb rgb;
    if (hex.size() == 3)
    {
        rgb.r = hexCharToInt(hex[0]) * 16 + hexCharToInt(hex[0]);
        rgb.g = hexCharToInt(hex[1]) * 16 + hexCharToInt(hex[1]);
        rgb.b = hexCharToInt(hex[2]) * 16 + hexCharToInt(hex[2]);
    }
    else if (hex.size() == 6)
    {
        rgb.r = hexCharToInt(hex[0]) * 16 + hexCharToInt(hex[1]);
        rgb.g = hexCharToInt(hex[2]) * 16 + hexCharToInt(hex[3]);
        rgb.b = hexCharToInt(hex[4]) * 16 + hexCharToInt(hex[5]);
    }
    else
    {
        throw std::invalid_argument("invalid hex color string '" + std::string(hex) + "'");
    }

    return rgb;
}

std::string rgbToHex(const Rgb& rgb, bool bigCase, bool withNumberSign)
{
    std::string hex(6, '0');
    hex[0] = intToHexChar(rgb.r / 16);
    hex[1] = intToHexChar(rgb.r % 16);
    hex[2] = intToHexChar(rgb.g / 16);
    hex[3] = intToHexChar(rgb.g % 16);
    hex[4] = intToHexChar(rgb.b / 16);
    hex[5] = intToHexChar(rgb.b % 16);
    return withNumberSign ? ("#" + hex) : hex;
}
