#include "version.hpp"

#include <cstdlib>

Version parseVersionString(std::string_view str)
{
    const int major = std::atoi(str.data());

    std::size_t pos = str.find('.') + 1;
    if (pos >= str.size()) return Version{};
    str = str.substr(pos);
    const int minor = std::atoi(str.data());

    pos = str.find('.') + 1;
    if (pos >= str.size()) return Version{};
    str = str.substr(pos);
    const int patch = std::atoi(str.data());

    return Version{
        static_cast<unsigned char>(major),
        static_cast<unsigned char>(minor),
        static_cast<unsigned char>(patch)};
}

std::string dumpVersionString(const Version& version)
{
    return
        std::to_string(static_cast<int>(version.major)) + "." +
        std::to_string(static_cast<int>(version.minor)) + "." +
        std::to_string(static_cast<int>(version.patch));
}
