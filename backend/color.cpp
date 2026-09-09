#include "color.hpp"

#include <assert.h>
#include <stdexcept>

static inline int hexCharToInt(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    throw std::invalid_argument("Rgb::fromHex(): invalid hex character '" + std::string(1, c) + "'");
}

Rgb Rgb::fromHex(std::string_view hex)
{
    if (hex.empty())
        throw std::invalid_argument("Rgb::fromHex(): empty hex color string");

    if (hex[0] == '#')
        hex = hex.substr(1);

    int r = 0, g = 0, b = 0;
    if (hex.size() == 3)
    {
        r = hexCharToInt(hex[0]) * 16 + hexCharToInt(hex[0]);
        g = hexCharToInt(hex[1]) * 16 + hexCharToInt(hex[1]);
        b = hexCharToInt(hex[2]) * 16 + hexCharToInt(hex[2]);
    }
    else if (hex.size() == 6)
    {
        r = hexCharToInt(hex[0]) * 16 + hexCharToInt(hex[1]);
        g = hexCharToInt(hex[2]) * 16 + hexCharToInt(hex[3]);
        b = hexCharToInt(hex[4]) * 16 + hexCharToInt(hex[5]);
    }
    else
    {
        throw std::invalid_argument("Rgb::fromHex(): invalid hex color string '" + std::string(hex) + "'");
    }

    return Rgb(
        static_cast<unsigned char>(r),
        static_cast<unsigned char>(g),
        static_cast<unsigned char>(b)
    );
}

static inline char intToHexChar(int i, bool uppercase)
{
    assert(i >= 0 && i <= 15);

    if (i >= 0  && i <= 9)  return i + '0';
    if (i >= 10 && i <= 15) return (uppercase ? (i - 10 + 'A') : (i - 10 + 'a'));
}

std::string Rgb::toHex(const Rgb& rgb, bool uppercase, bool prefixed) const
{
    std::string hex(6, '0');
    hex[0] = intToHexChar(rgb.r / 16, uppercase);
    hex[1] = intToHexChar(rgb.r % 16, uppercase);
    hex[2] = intToHexChar(rgb.g / 16, uppercase);
    hex[3] = intToHexChar(rgb.g % 16, uppercase);
    hex[4] = intToHexChar(rgb.b / 16, uppercase);
    hex[5] = intToHexChar(rgb.b % 16, uppercase);
    return prefixed ? ("#" + hex) : hex;
}
