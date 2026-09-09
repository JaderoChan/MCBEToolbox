#pragma once

#include <string>
#include <string_view>

#pragma pack(push, 1)
/** 8 位 RGB 颜色值 */
struct Rgb
{
    static_assert(sizeof(unsigned char) == 1, "sizeof(unsigned char) != 1");

    constexpr Rgb() noexcept = default;
    constexpr Rgb(unsigned char r, unsigned char g, unsigned char b) noexcept : r(r), g(g), b(b) {}

    /**
     * 从 "#000"/"#000000"/"000"/"000000" 格式的字符串中解析 RGB 颜色值。
     *
     * @throw invalid_argument
     */
    static Rgb fromHex(std::string_view hex);

    /**
     * 将 RGB 颜色值转换为十六进制颜色值字符串。
     *
     * @param uppercase 使用大写的十六进制字符
     * @param prefixed  以井号 '#' 作为字符串前缀
     */
    std::string toHex(const Rgb& rgb, bool uppercase = true, bool prefixed = true) const;

    unsigned char r = 0, g = 0, b = 0;
};
#pragma pack(pop)
static_assert(sizeof(Rgb) == 3, "sizeof(Rgb) != 3");

static inline constexpr bool operator==(const Rgb& lhs, const Rgb& rhs)
{
    return (lhs.r == rhs.r) && (lhs.g == rhs.g) && (lhs.b == rhs.b);
}

namespace std
{

template<>
struct hash<Rgb>
{
    static_assert(sizeof(unsigned int) >= 3, "sizeof(unsigned int) < 3");

    std::size_t operator()(const Rgb& rgb) const
    {
        const unsigned int v =
            (static_cast<unsigned int>(rgb.r) << 16) |
            (static_cast<unsigned int>(rgb.g) << 8)  |
            (static_cast<unsigned int>(rgb.b));
        return std::hash<unsigned int>()(v);
    }
};

} // namespace std
