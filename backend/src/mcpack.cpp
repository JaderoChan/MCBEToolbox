#include "mcpack.hpp"

#include <cstdio>       // fprintf
#include <filesystem>   // std::filesystem
#include <fstream>      // std::ifstream, std::ofstream
#include <random>       // std::mt19937_64, std::random_device, std::uniform_int_distribution
#include <system_error> // std::error_code

#include <mz.h>          // MZ_OK, MZ_COMPRESS_METHOD_DEFLATE, MZ_COMPRESS_LEVEL_DEFAULT
#include <mz_strm.h>     // mz_stream_read_cb, mz_stream_write_cb
#include <mz_zip.h>      // mz_zip_file
#include <mz_zip_rw.h>   // mz_zip_writer_*
#include <nlohmann/json.hpp>     // nlohmann::json
#include <opencv2/imgcodecs.hpp> // cv::imwrite

#include <image_utilities.hpp>
#include "color.hpp"

#ifdef _WIN32
    #include <windows.h> // SetFileAttributesW
#endif

namespace
{

// 便利函数：确保给定目录存在，不存在则创建，已存在但不是目录则失败。
bool ensureDirectoryExists(const std::filesystem::path& dirPath)
{
    std::error_code ec;
    if (std::filesystem::exists(dirPath, ec))
        return std::filesystem::is_directory(dirPath, ec);

    std::filesystem::create_directories(dirPath, ec);
    return !ec;
}

// pack_icon.png 默认图像调色板
constexpr Rgb PACK_ICON_PALETTE[4] = {
    Rgb(96, 161, 74),  // 草方块 - 亮
    Rgb(83, 145, 62),  // 草方块 - 暗
    Rgb(134, 96, 67),  // 泥土   - 亮
    Rgb(112, 79, 54)   // 泥土   - 暗
};

constexpr unsigned char PACK_ICON_PIXELS[16][16] = {
    { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
    { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 },
    { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
    { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 },
    { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
    { 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2 },
    { 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3 },
    { 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2 },
    { 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3 },
    { 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2 },
    { 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3 },
    { 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2 },
    { 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3 },
    { 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2 },
    { 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3 },
    { 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2 }
};

// 基于 PACK_ICON_PALETTE/PACK_ICON_PIXELS 生成默认 pack_icon.png 图像。
const cv::Mat& defaultPackIconImage()
{
    static const cv::Mat icon = []
    {
        cv::Mat img(16, 16, CV_8UC3);
        for (int y = 0; y < 16; ++y)
        {
            for (int x = 0; x < 16; ++x)
            {
                const Rgb& color = PACK_ICON_PALETTE[PACK_ICON_PIXELS[y][x]];
                img.at<cv::Vec3b>(y, x) = cv::Vec3b(color.b, color.g, color.r);
            }
        }

        return resizeImage(img, cv::Size(256, 256));
    }();
    return icon;
}

} // namespace

MCPack::MCPack(
    std::string_view filePath,
    std::string_view name,
    std::string_view description,
    std::string_view _namespace,
    const Version&   version)
    : filePath_(filePath), name_(name), description_(description)
    , namespace_(_namespace), version_(version)
{
    if (filePath_.empty())
    {
        fprintf(stderr, "MCPack::MCPack() Parameter 'filePath' is empty\n");
        return;
    }

    tempPath_ = generateTempPath();
    if (!isOk_)
        return;

    if (!ensureDirectoryExists(std::filesystem::path(tempPath_) / "functions"  / namespace_) ||
        !ensureDirectoryExists(std::filesystem::path(tempPath_) / "structures" / namespace_))
    {
        fprintf(stderr, "MCPack::MCPack() Failed to create the functions/structures directory\n");
        isOk_ = false;
        return;
    }

    const std::filesystem::path tickJsonPath = std::filesystem::path(tempPath_) / "functions" / "tick.json";
    std::ofstream tickJsonFile(tickJsonPath, std::ios::binary | std::ios::trunc);
    if (!tickJsonFile)
    {
        fprintf(stderr, "MCPack::MCPack() Failed to create 'tick.json'\n");
        isOk_ = false;
        return;
    }
    tickJsonFile << nlohmann::json{{"values", nlohmann::json::array()}}.dump(4);
    tickJsonFile.close();
}

MCPack::~MCPack()
{
    removeTempPath();
}

std::string MCPack::getFunctionsPath() const
{
    return (std::filesystem::path(tempPath_) / "functions" / namespace_).string();
}

std::string MCPack::getStructuresPath() const
{
    return (std::filesystem::path(tempPath_) / "structures" / namespace_).string();
}

bool MCPack::addFunction(std::string_view relativePath, const std::vector<std::string>& mcfunction) const
{
    if (!isOk_ || relativePath.empty())
        return false;

    const std::filesystem::path path =
        std::filesystem::path(getFunctionsPath()) / (std::string(relativePath) + ".mcfunction");
    if (!ensureDirectoryExists(path.parent_path()))
    {
        fprintf(stderr, "MCPack::addFunction() Failed to create the parent directory of '%s'\n", path.string().c_str());
        return false;
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file)
    {
        fprintf(stderr, "MCPack::addFunction() Failed to create the function file '%s'\n", path.string().c_str());
        return false;
    }

    for (const std::string& command : mcfunction)
        file << command << '\n';
    file.close();
    return true;
}

bool MCPack::addTickFunction(std::string_view relativePath) const
{
    if (!isOk_ || relativePath.empty())
        return false;

    const std::filesystem::path tickJsonPath = std::filesystem::path(tempPath_) / "functions" / "tick.json";

    std::ifstream inFile(tickJsonPath, std::ios::binary);
    if (!inFile)
    {
        fprintf(stderr, "MCPack::addTickFunction() Failed to open 'tick.json'\n");
        return false;
    }

    nlohmann::json json;
    try
    {
        inFile >> json;
    }
    catch (const nlohmann::json::exception& e)
    {
        fprintf(stderr, "MCPack::addTickFunction() Failed to parse 'tick.json': %s\n", e.what());
        return false;
    }
    inFile.close();

    nlohmann::json& values = json["values"];
    if (!values.is_array())
        values = nlohmann::json::array();

    const std::string entry = namespace_ + "/" + std::string(relativePath);
    for (const auto& value : values)
    {
        if (value.is_string() && value.get<std::string>() == entry)
            return true;
    }
    values.push_back(entry);

    std::ofstream outFile(tickJsonPath, std::ios::binary | std::ios::trunc);
    if (!outFile)
    {
        fprintf(stderr, "MCPack::addTickFunction() Failed to write 'tick.json'\n");
        return false;
    }
    outFile << json.dump(4);
    outFile.close();
    return true;
}

bool MCPack::pack() const
{
    if (!isOk_)
        return false;

    const nlohmann::json manifest = {
        {"format_version", formatVersion_},
        {"header", {
            {"description", description_},
            {"name", name_},
            {"uuid", generateUuid()},
            {"version", {version_.major, version_.minor, version_.patch}},
            {"min_engine_version", {minVersion_.major, minVersion_.minor, minVersion_.patch}}
        }},
        {"modules", nlohmann::json::array({
            {
                {"description", description_},
                {"type", "data"},
                {"uuid", generateUuid()},
                {"version", {version_.major, version_.minor, version_.patch}}
            }
        })}
    };

    const std::filesystem::path manifestPath = std::filesystem::path(tempPath_) / "manifest.json";
    std::ofstream manifestFile(manifestPath, std::ios::binary | std::ios::trunc);
    if (!manifestFile)
    {
        fprintf(stderr, "MCPack::pack() Failed to create 'manifest.json'\n");
        return false;
    }
    manifestFile << manifest.dump(4);
    manifestFile.close();

    const std::filesystem::path iconPath = std::filesystem::path(tempPath_) / "pack_icon.png";
    if (!cv::imwrite(iconPath.string(), defaultPackIconImage()))
    {
        fprintf(stderr, "MCPack::pack() Failed to create 'pack_icon.png'\n");
        return false;
    }

    if (!ensureDirectoryExists(filePath_))
    {
        fprintf(stderr, "MCPack::pack() Failed to create the target directory '%s'\n", filePath_.c_str());
        return false;
    }

    const std::filesystem::path zipPath = std::filesystem::path(filePath_) / (name_ + ".mcpack");
    std::error_code ec;
    std::filesystem::remove(zipPath, ec);

    void* writer = mz_zip_writer_create();
    if (!writer)
    {
        fprintf(stderr, "MCPack::pack() Failed to create the zip writer\n");
        return false;
    }

    mz_zip_writer_set_compress_method(writer, MZ_COMPRESS_METHOD_DEFLATE);
    mz_zip_writer_set_compress_level(writer, MZ_COMPRESS_LEVEL_DEFAULT);

    const std::string zipPathUtf8  = zipPath.u8string();
    const std::string tempPathUtf8 = std::filesystem::path(tempPath_).u8string();

    bool ok = mz_zip_writer_open_file(writer, zipPathUtf8.c_str(), 0, 0) == MZ_OK;
    if (ok)
        ok = mz_zip_writer_add_path(writer, tempPathUtf8.c_str(), tempPathUtf8.c_str(), 0, 1) == MZ_OK;
    ok = (mz_zip_writer_close(writer) == MZ_OK) && ok;

    mz_zip_writer_delete(&writer);

    if (!ok)
        fprintf(stderr, "MCPack::pack() Failed to compress the pack contents into '%s'\n", zipPath.string().c_str());

    return ok;
}

std::string MCPack::generateUuid()
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

std::string MCPack::generateTempPath()
{
    const std::filesystem::path anchor(filePath_);
    const std::filesystem::path parent =
        anchor.has_parent_path()
        ? anchor.parent_path()
        : std::filesystem::current_path();

    const std::string hiddenName = "." + anchor.filename().string() + "_" + generateUuid() + ".tmp";
    const std::filesystem::path tempPath = parent / hiddenName;

    std::error_code ec;
    if (std::filesystem::exists(tempPath, ec))
    {
        fprintf(
            stderr,
            "MCPack::generateTempPath() Temporary directory '%s' already exists\n",
            tempPath.string().c_str()
        );
        isOk_ = false;
        return std::string();
    }

    if (!std::filesystem::create_directories(tempPath, ec) || ec)
    {
        fprintf(
            stderr,
            "MCPack::generateTempPath() Failed to create the temporary directory '%s'\n",
            tempPath.string().c_str()
        );
        isOk_ = false;
        return std::string();
    }

#ifdef _WIN32
    SetFileAttributesW(tempPath.wstring().c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif

    isOk_ = true;
    return tempPath.string();
}

void MCPack::removeTempPath() const
{
    if (tempPath_.empty())
        return;

    std::error_code ec;
    std::filesystem::remove_all(tempPath_, ec);
}
