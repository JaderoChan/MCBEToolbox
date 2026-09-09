#include "version.hpp"

#include <stdint.h> // UINT8_MAX
#include <stdexcept>

Version Version::fromString(std::string_view str)
{
    if (str.empty())
        throw std::invalid_argument("Version::fromString(): empty version string");

#define THROW_INVALID_VERSION_STRING \
throw std::invalid_argument("Version::fromString(): invalid version string '" + std::string(str) + "'")

    int  values[4] = {0};   // 各个字段的值
    int  pos       = 0;     // 当前正在解析的字段下标（0~3）
    bool hasDigit  = false; // 当前字段是否至少读到一位数字

    for (char c : str)
    {
        if (c >= '0' && c <= '9')
        {
            if (pos == 4) THROW_INVALID_VERSION_STRING;
            values[pos] = values[pos] * 10 + static_cast<unsigned int>(c - '0');
            if (values[pos] > UINT8_MAX) THROW_INVALID_VERSION_STRING;
            hasDigit = true;
        }
        else if (c == '.')
        {
            if (!hasDigit) THROW_INVALID_VERSION_STRING;
            if (++pos == 4) THROW_INVALID_VERSION_STRING;
            hasDigit = false;
        }
        else
        {
            THROW_INVALID_VERSION_STRING;
        }
    }

    if (!hasDigit) THROW_INVALID_VERSION_STRING;

    return Version(
        static_cast<unsigned char>(values[0]),
        static_cast<unsigned char>(values[1]),
        static_cast<unsigned char>(values[2]),
        static_cast<unsigned char>(values[3])
    );

#undef THROW_INVALID_VERSION_STRING
}

std::string Version::toString() const
{
    return
        std::to_string(static_cast<int>(major)) + "." +
        std::to_string(static_cast<int>(minor)) + "." +
        std::to_string(static_cast<int>(patch)) + "." +
        std::to_string(static_cast<int>(tweak));
}
