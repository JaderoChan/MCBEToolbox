#pragma once

#include <string>
#include <string_view>

#pragma pack(push, 1)
/** 版本号（字段最大值为 255） */
struct Version
{
    static_assert(sizeof(unsigned char) == 1, "sizeof(unsigned char) != 1");
    static_assert(sizeof(unsigned int)  == 4, "sizeof(unsigned int) != 4");

    constexpr Version() noexcept = default;
    constexpr Version(unsigned char major, unsigned char minor, unsigned char patch, unsigned char tweak = 0) noexcept
        : major(major), minor(minor), patch(patch), tweak(tweak) {}

    /**
     * 从 "major.minor.patch"/"major.minor.patch.tweak" 格式的字符串中解析版本号。
     *
     * @throw invalid_argument
     */
    static Version fromString(std::string_view str);

    /** 将版本号序列化为 "major.minor.patch.tweak" 格式的字符串。 */
    std::string toString() const;

    /** 将版本号转换为无符号整数值。 */
    unsigned int toUInt32() const
    {
        return
            (static_cast<unsigned int>(major) << 24) |
            (static_cast<unsigned int>(minor) << 16) |
            (static_cast<unsigned int>(patch) << 8)  |
            (static_cast<unsigned int>(tweak));
    }

    unsigned char major = 0, minor = 0, patch = 0, tweak = 0;
};
#pragma pack(pop)
static_assert(sizeof(Version) == 4, "sizeof(Version) != 4");

static inline constexpr bool operator==(const Version& lhs, const Version& rhs)
{ return lhs.toUInt32() == rhs.toUInt32(); }

static inline constexpr bool operator<(const Version& lhs, const Version& rhs)
{ return lhs.toUInt32() < rhs.toUInt32(); }

static inline constexpr bool operator>(const Version& lhs, const Version& rhs)
{ return lhs.toUInt32() > rhs.toUInt32(); }

static inline constexpr bool operator<=(const Version& lhs, const Version& rhs)
{ return (lhs < rhs ) || (lhs == rhs); }

static inline constexpr bool operator>=(const Version& lhs, const Version& rhs)
{ return (lhs > rhs ) || (lhs == rhs); }

namespace std
{

template<>
struct hash<Version>
{
    std::size_t operator()(const Version& version) const
    {
        return std::hash<unsigned int>()(version.toUInt32());
    }
};

} // namespace std
