#include "version.hpp"

#include <stdexcept>

Version parseVersionString(std::string_view str)
{
    if (str.empty())
        throw std::invalid_argument("empty version string");

    const std::size_t pos1 = str.find('.');
    if (pos1 == std::string_view::npos)
        throw std::invalid_argument("invalid version string '" + std::string(str) + "'");
    const std::size_t pos2 = str.find('.', pos1 + 1);
    if (pos2 == std::string_view::npos)
        throw std::invalid_argument("invalid version string '" + std::string(str) + "'");

    std::string_view majorStr = str.substr(0, pos1);
    std::string_view minorStr = str.substr(pos1 + 1, pos2 - pos1 - 1);
    std::string_view patchStr = str.substr(pos2 + 1);
    const int major = std::stoi(std::string(majorStr));
    const int minor = std::stoi(std::string(minorStr));
    const int patch = std::stoi(std::string(patchStr));

    if ((major >= 0 && major <= UINT8_MAX) &&
        (minor >= 0 && minor <= UINT8_MAX) &&
        (patch >= 0 && patch <= UINT8_MAX))
    {
        return Version{
            static_cast<unsigned char>(major),
            static_cast<unsigned char>(minor),
            static_cast<unsigned char>(patch)};
    }
    else
    {
        throw std::invalid_argument("invalid version string '" + std::string(str) + "'");
    }
}

std::string dumpVersionString(const Version& vers)
{
    return
        std::to_string(static_cast<int>(vers.major)) + "." +
        std::to_string(static_cast<int>(vers.minor)) + "." +
        std::to_string(static_cast<int>(vers.patch));
}
