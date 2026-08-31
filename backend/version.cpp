#include "version.hpp"

#include <stdlib.h>

Version parseVersionString(std::string str)
{
    if (str.empty()) return Version{0, 0, 0};

    const int major = atoi(str.data());

    std::size_t pos = str.find('.');
    if (pos == std::string::npos) return Version{};
    str = str.substr(pos + 1);
    const int minor = atoi(str.data());

    pos = str.find('.');
    if (pos == std::string::npos) return Version{};
    str = str.substr(pos + 1);
    const int patch = atoi(str.data());

    return Version{
        static_cast<unsigned char>(major),
        static_cast<unsigned char>(minor),
        static_cast<unsigned char>(patch)};
}

std::string dumpVersionString(const Version& vers)
{
    return
        std::to_string(static_cast<int>(vers.major)) + "." +
        std::to_string(static_cast<int>(vers.minor)) + "." +
        std::to_string(static_cast<int>(vers.patch));
}
