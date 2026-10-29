#pragma once

#include <filesystem>  // std::filesystem::path
#include <string>      // std::string
#include <string_view> // std::string_view

namespace fs_utils
{

/** 生成一个随机的 UUID v4 字符串。 */
std::string generateUuid4();

/** 将 UTF-8 字符串转换为 std::filesystem::path。 */
std::filesystem::path pathFromUtf8(std::string_view utf8);

/**
 * 在 anchorPath 所在目录下创建一个全局唯一的隐藏临时目录。
 *
 * @return 创建成功时返回临时目录路径，失败时返回空路径。
 */
std::filesystem::path createUniqueHiddenTempDir(const std::filesystem::path& anchorPath);

/** 递归删除给定目录及其所有内容，忽略错误（路径为空时直接返回）。 */
void removeDirectoryRecursive(const std::filesystem::path& dirPath);

/** 确保给定目录存在，不存在则创建；已存在但不是目录则视为失败。 */
bool ensureDirectoryExists(const std::filesystem::path& dirPath);

/**
 * 将 src 移动至 dst（若 dst 已存在则先删除），同设备下为原子重命名，跨设备重命名失败时则回退为复制 + 删除源文件。
 *
 * @return 是否成功，src 不存在时也视为失败。
 */
bool moveFileReplacing(const std::filesystem::path& src, const std::filesystem::path& dst);

/** RAII 路径清理，析构时递归删除给定路径（文件或目录）。 */
class ScopedPathRemover
{
public:
    explicit ScopedPathRemover(std::filesystem::path path) : path_(std::move(path)) {}
    ~ScopedPathRemover() { removeDirectoryRecursive(path_); }

    ScopedPathRemover(const ScopedPathRemover&)            = delete;
    ScopedPathRemover& operator=(const ScopedPathRemover&) = delete;

private:
    std::filesystem::path path_;
};

#ifdef _WIN32
/**
 * 为“只接受窄字符串路径、且内部按本地 ANSI 代码页解释该路径”的旧式 C/C++ API
 * （如 OpenCV 的 cv::VideoCapture/cv::VideoWriter）提供的安全访问手段：
 *
 * 在临时目录下创建一个指向原文件的符号链接。
 * 符号链接会在对象析构时自动删除，因此必须保证其生命周期覆盖旧式 API 实际持有/读取该文件
 * 的整个期间（例如 cv::VideoCapture 是逐帧从磁盘读取的，不能在 open() 后立即销毁别名）。
 *
 * 仅在 Windows 上使用。
 */
class AsciiSafeAlias
{
public:
    explicit AsciiSafeAlias(const std::filesystem::path& originalPath);
    ~AsciiSafeAlias();

    AsciiSafeAlias(const AsciiSafeAlias&)            = delete;
    AsciiSafeAlias& operator=(const AsciiSafeAlias&) = delete;

    /** 是否已成功创建别名。 */
    bool ok() const { return ok_; }

    /** 获取可安全传给旧式窄字符串 API 的路径：别名创建成功时为别名路径，否则回退为原路径。 */
    const std::filesystem::path& path() const { return ok_ ? aliasPath_ : originalPath_; }

private:
    std::filesystem::path originalPath_;
    std::filesystem::path aliasPath_;
    bool                  ok_ = false;
};
#endif // _WIN32

} // namespace fs_utils
