#include "mcpack.hpp"

#include <stdio.h>      // fprintf
#include <filesystem>   // std::filesystem
#include <fstream>      // std::ifstream, std::ofstream
#include <random>       // std::mt19937_64, std::random_device, std::uniform_int_distribution
#include <system_error> // std::error_code

#include <mz.h>          // MZ_OK, MZ_COMPRESS_METHOD_DEFLATE, MZ_COMPRESS_LEVEL_DEFAULT
#include <mz_strm.h>     // mz_stream_read_cb, mz_stream_write_cb
#include <mz_zip.h>      // mz_zip_file
#include <mz_zip_rw.h>   // mz_zip_writer_*
#include <nlohmann/json.hpp> // nlohmann::json

#include "pack_icon_data.hpp"

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

// 便利函数：递归判断给定目录下是否不包含任何常规文件。
bool isDirectoryEmptyOfFiles(const std::filesystem::path& dirPath)
{
    std::error_code ec;
    if (!std::filesystem::is_directory(dirPath, ec))
        return true;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(
             dirPath, std::filesystem::directory_options::skip_permission_denied, ec))
    {
        if (entry.is_regular_file(ec))
            return false;
    }
    return true;
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
    std::ofstream iconFile(iconPath, std::ios::binary | std::ios::trunc);
    if (!iconFile)
    {
        fprintf(stderr, "MCPack::pack() Failed to create 'pack_icon.png'\n");
        return false;
    }
    iconFile.write(reinterpret_cast<const char*>(DEFAULT_PACK_ICON_PNG), DEFAULT_PACK_ICON_PNG_SIZE);
    iconFile.close();

    const std::filesystem::path functionsRootPath       = std::filesystem::path(tempPath_) / "functions";
    const std::filesystem::path functionsNamespacePath  = functionsRootPath / namespace_;
    const std::filesystem::path structuresRootPath      = std::filesystem::path(tempPath_) / "structures";
    const std::filesystem::path structuresNamespacePath = structuresRootPath / namespace_;

    std::error_code ec;
    if (isDirectoryEmptyOfFiles(functionsNamespacePath))
        std::filesystem::remove_all(functionsRootPath, ec);
    if (isDirectoryEmptyOfFiles(structuresNamespacePath))
        std::filesystem::remove_all(structuresRootPath, ec);

    const std::filesystem::path zipPath = filePath_;
    const std::filesystem::path zipParentPath =
        zipPath.has_parent_path() ? zipPath.parent_path() : std::filesystem::current_path();
    if (!ensureDirectoryExists(zipParentPath))
    {
        fprintf(stderr, "MCPack::pack() Failed to create the target directory '%s'\n", zipParentPath.string().c_str());
        return false;
    }

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

    int32_t err = mz_zip_writer_open_file(writer, zipPathUtf8.c_str(), 0, 0);
    if (err != MZ_OK)
        fprintf(stderr, "MCPack::pack() Failed to open the zip file '%s' (error %d)\n", zipPath.string().c_str(), err);

    if (err == MZ_OK)
    {
        err = mz_zip_writer_add_path(writer, tempPathUtf8.c_str(), tempPathUtf8.c_str(), 0, 1);
        if (err != MZ_OK)
            fprintf(stderr, "MCPack::pack() Failed to add the pack contents to the zip (error %d)\n", err);
    }

    const int32_t closeErr = mz_zip_writer_close(writer);
    if (closeErr != MZ_OK)
        fprintf(stderr, "MCPack::pack() Failed to close the zip writer (error %d)\n", closeErr);

    mz_zip_writer_delete(&writer);

    return err == MZ_OK && closeErr == MZ_OK;
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

    const std::string hiddenName = "." + anchor.stem().string() + "_" + generateUuid() + ".tmp";
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
