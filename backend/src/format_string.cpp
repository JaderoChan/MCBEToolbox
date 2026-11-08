#include <format_string.hpp>

std::string escapeQuotes(const std::string& str)
{
    std::string result;
    result.reserve(str.size());

    std::size_t pos = 0;
    while (pos < str.size())
    {
        std::size_t next = str.find_first_of("\"", pos);
        if (next == std::string::npos)
        {
            result.append(str, pos, std::string::npos);
            break;
        }
        result.append(str, pos, next - pos);
        result += '\\';
        result += str[next];
        pos = next + 1;
    }

    return result;
}
