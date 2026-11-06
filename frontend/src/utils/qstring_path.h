#pragma once

#include <filesystem>

#include <qstring.h>

inline std::filesystem::path toFsPath(const QString& str)
{
#ifdef _WIN32
    return std::filesystem::path(str.toStdWString());
#else
    return std::filesystem::path(str.toStdString());
#endif // _WIN32
}
