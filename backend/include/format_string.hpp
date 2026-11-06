#pragma once

#include <string>
#include <sstream>

namespace format_string_impl
{

inline void formatStringImpl(std::ostringstream& oss, const std::string& fmt, std::size_t pos)
{
    oss << fmt.substr(pos);
}

template <typename T, typename... Args>
void formatStringImpl(std::ostringstream& oss, const std::string& fmt, std::size_t pos, T&& value, Args&&... args)
{
    while (pos < fmt.size())
    {
        if (fmt[pos] == '{')
        {
            if (pos + 1 < fmt.size() && fmt[pos + 1] == '{')
            {
                oss << '{';
                pos += 2;
                continue;
            }

            if (pos + 1 < fmt.size() && fmt[pos + 1] == '}')
            {
                oss << value;
                formatStringImpl(oss, fmt, pos + 2, std::forward<Args>(args)...);
                return;
            }

            oss << fmt[pos];
            ++pos;
        }
        else if (fmt[pos] == '}')
        {
            if (pos + 1 < fmt.size() && fmt[pos + 1] == '}')
            {
                oss << '}';
                pos += 2;
            }
            else
            {
                oss << fmt[pos];
                ++pos;
            }
        }
        else
        {
            oss << fmt[pos];
            ++pos;
        }
    }
}

} // format_string_impl

template <typename... Args>
std::string formatString(const std::string& fmt, Args&&... args)
{
    std::ostringstream oss;
    format_string_impl::formatStringImpl(oss, fmt, 0, std::forward<Args>(args)...);
    return oss.str();
}
