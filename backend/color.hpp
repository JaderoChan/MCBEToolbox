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

/**
 * 从格式为 “#000”、“#000000”、“000”、“000000” 的字符串中解析 RGB 颜色值。
 *
 * @throw invalid_argument
 */
Rgb hexToRgb(std::string_view hex);

/**
 * 获取字符串形式的 RGB 颜色值。
 *
 * @param isUpperCase     使用大写的十六进制字符串
 * @param hasNumberSign 以井号 `#` 作为字符串前缀。
 */
std::string rgbToHex(const Rgb& rgb, bool isUpperCase = true, bool hasNumberSign = true);
