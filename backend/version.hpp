#pragma once

#include <string>
#include <string_view>

struct Version
{
    static_assert(sizeof(unsigned char) == 1, "sizeof(unsigned char) != 1");
    static_assert(sizeof(unsigned int)  >= 3, "sizeof(unsigned int) < 3");

    Version() : major(0), minor(0), patch(0) {}
    Version(unsigned char major, unsigned char minor, unsigned char patch)
        : major(major), minor(minor), patch(patch) {}

    /**
     * 从 "major.minor.patch" 格式的字符串中解析版本号。
     *
     * @throw invalid_argument
     */
    static Version fromString(std::string_view str);

    /** 将版本号序列化为 "major.minor.patch" 格式的字符串。 */
    std::string toString() const;

    /** 获取版本号的哈希值，同时可用于比较版本大小。 */
    unsigned int hash() const
    {
        return
            (static_cast<unsigned int>(major) << 16) |
            (static_cast<unsigned int>(minor) << 8)  |
            (static_cast<unsigned int>(patch));
    }

    unsigned char major, minor, patch;
};

static inline bool operator==(const Version& lhs, const Version& rhs)
{
    return lhs.hash() == rhs.hash();
}

static inline bool operator<(const Version& lhs, const Version& rhs)
{
    return lhs.hash() < rhs.hash();
}

static inline bool operator>(const Version& lhs, const Version& rhs)
{
    return lhs.hash() > rhs.hash();
}

static inline bool operator<=(const Version& lhs, const Version& rhs)
{
    return (lhs < rhs ) || (lhs == rhs);
}

static inline bool operator>=(const Version& lhs, const Version& rhs)
{
    return (lhs > rhs ) || (lhs == rhs);
}

namespace std
{

template<>
struct hash<Version>
{
    std::size_t operator()(const Version& version) const
    {
        return std::hash<unsigned int>()(version.hash());
    }
};

} // namespace std
