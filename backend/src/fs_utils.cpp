#include "fs_utils.hpp"

#include <stdio.h>      // fprintf
#include <random>       // std::mt19937_64, std::random_device, std::uniform_int_distribution
#include <system_error> // std::error_code

#ifdef _WIN32
    #include <windows.h>  // SetFileAttributesW
#elif defined(__APPLE__)
    #include <sys/stat.h> // chflags, UF_HIDDEN
#endif

namespace fs_utils
{

std::string generateUuid4()
{
    static thread_local std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 15);
    static const char hex[] = "0123456789abcdef";

    char buf[37];
    int  pos = 0;
    for (int i = 0; i < 36; ++i)
    {
        if (i == 8 || i == 13 || i == 18 || i == 23)
        {
            buf[pos++] = '-';
            continue;
        }

        int value = dist(rng);
        if (i == 14)
            value = 4;                   // UUID version 4
        else if (i == 19)
            value = (value & 0x3) | 0x8; // UUID variant 1
        buf[pos++] = hex[value];
    }
    buf[pos] = '\0';

    return std::string(buf);
}

std::filesystem::path pathFromUtf8(std::string_view utf8)
{
    return std::filesystem::u8path(utf8.begin(), utf8.end());
}

bool ensureDirectoryExists(const std::filesystem::path& dirPath)
{
    std::error_code ec;
    if (std::filesystem::exists(dirPath, ec))
        return std::filesystem::is_directory(dirPath, ec);
    std::filesystem::create_directories(dirPath, ec);
    return !ec;
}

std::filesystem::path createUniqueHiddenTempDir(const std::filesystem::path& anchorPath)
{
    const std::filesystem::path parent =
        anchorPath.has_parent_path() ? anchorPath.parent_path() : std::filesystem::current_path();

    std::error_code ec;
    // 重试几次以应对 UUID 碰撞或目录已被占用。
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        const std::string           hiddenName = "." + anchorPath.string() + "_" + generateUuid4() + ".tmp";
        const std::filesystem::path tempPath   = parent / hiddenName;

        if (std::filesystem::exists(tempPath, ec))
            continue;

        if (std::filesystem::create_directories(tempPath, ec) && !ec)
        {
            // 隐藏文件夹。
        #ifdef _WIN32
            SetFileAttributesW(tempPath.wstring().c_str(), FILE_ATTRIBUTE_HIDDEN);
        #elif defined(__APPLE__)
            chflags(tempPath.c_str(), UF_HIDDEN);
        #endif
            return tempPath;
        }
    }

    fprintf(
        stderr,
        "fs_utils::createUniqueHiddenTempDir() Failed to create a unique temporary directory near '%s'\n",
        anchorPath.string().c_str()
    );
    return std::filesystem::path();
}

void removeDirectoryRecursive(const std::filesystem::path& dirPath)
{
    if (dirPath.empty())
        return;

    std::error_code ec;
    std::filesystem::remove_all(dirPath, ec);
}

bool moveFileReplacing(const std::filesystem::path& src, const std::filesystem::path& dst)
{
    std::error_code ec;
    if (!std::filesystem::exists(src, ec))
    {
        fprintf(stderr, "fs_utils::moveFileReplacing() Source '%s' does not exist\n", src.string().c_str());
        return false;
    }

    const std::filesystem::path dstParent =
        dst.has_parent_path() ? dst.parent_path() : std::filesystem::current_path();
    if (!ensureDirectoryExists(dstParent))
    {
        fprintf(
            stderr,
            "fs_utils::moveFileReplacing() Failed to create the directory '%s'\n",
            dstParent.string().c_str()
        );
        return false;
    }

    std::filesystem::remove_all(dst, ec);

    ec.clear();
    std::filesystem::rename(src, dst, ec);
    if (!ec)
        return true;

    // 如果重命名失败，回退为复制 + 删除源文件。
    ec.clear();
    if (std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing, ec))
    {
        std::filesystem::remove(src, ec);
        return true;
    }

    fprintf(
        stderr,
        "fs_utils::moveFileReplacing() Failed to move '%s' to '%s'\n",
        src.string().c_str(), dst.string().c_str()
    );
    return false;
}

#ifdef _WIN32

AsciiSafeAlias::AsciiSafeAlias(const std::filesystem::path& originalPath) : originalPath_(originalPath)
{
    std::error_code ec;
    if (!std::filesystem::exists(originalPath_, ec))
        return;

    const std::filesystem::path absOriginal = std::filesystem::absolute(originalPath_, ec);
    if (ec)
        return;
    const std::filesystem::path root = absOriginal.root_path();
    const std::filesystem::path candidate = root / (generateUuid4() + originalPath_.extension().string());

    ec.clear();
    std::filesystem::create_symlink(absOriginal, candidate, ec);
    if (!ec)
    {
        aliasPath_ = candidate;
        ok_        = true;
        return;
    }

    // 如果创建符号链接失败，回退为复制一份临时文件。
    ec.clear();
    if (std::filesystem::copy_file(absOriginal, candidate, ec) && !ec)
    {
        aliasPath_ = candidate;
        ok_        = true;
        return;
    }

    fprintf(
        stderr,
        "fs_utils::AsciiSafeAlias::AsciiSafeAlias() Failed to create an ASCII-safe alias for '%s'\n",
        originalPath_.string().c_str()
    );
}

AsciiSafeAlias::~AsciiSafeAlias()
{
    if (!ok_)
        return;

    std::error_code ec;
    std::filesystem::remove(aliasPath_, ec);
}

#endif // _WIN32

} // namespace fs_utils
