#pragma once

#include <string>
#include <string_view>

struct Version
{
    static_assert(sizeof(unsigned char) == 1, "sizeof(unsigned char) != 1");
    static_assert(sizeof(int)           >= 3, "sizeof(int) < 3");

    unsigned char major, minor, patch;
};

inline bool operator==(const Version& lhs, const Version& rhs)
{
    return (lhs.major == rhs.major) && (lhs.minor == rhs.minor) && (lhs.patch == rhs.patch);
}

inline bool operator<(const Version& lhs, const Version& rhs)
{
    const int lv = (
        (static_cast<int>(lhs.major) << 16) |
        (static_cast<int>(lhs.minor) << 8)  |
        (static_cast<int>(lhs.patch)));
    const int rv = (
        (static_cast<int>(rhs.major) << 16) |
        (static_cast<int>(rhs.minor) << 8)  |
        (static_cast<int>(rhs.patch)));
    return lv < rv;
}

inline bool operator>(const Version& lhs, const Version& rhs)
{
    const int lv = (
        (static_cast<int>(lhs.major) << 16) |
        (static_cast<int>(lhs.minor) << 8)  |
        (static_cast<int>(lhs.patch)));
    const int rv = (
        (static_cast<int>(rhs.major) << 16) |
        (static_cast<int>(rhs.minor) << 8)  |
        (static_cast<int>(rhs.patch)));
    return lv > rv;
}

inline bool operator<=(const Version& lhs, const Version& rhs)
{
    return (lhs < rhs ) || (lhs == rhs);
}

inline bool operator>=(const Version& lhs, const Version& rhs)
{
    return (lhs > rhs ) || (lhs == rhs);
}

/**
 * 从格式为 “major.minor.patch” 的字符串中解析版本号。
 *
 * @return 解析失败或出错时返回 `Version{0, 0, 0}`。
 */
Version parseVersionString(std::string_view str);

/** 获取字符串形式的版本号。 */
std::string dumpVersionString(const Version& version);
