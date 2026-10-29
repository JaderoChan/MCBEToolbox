#include "mcpack.hpp"

#include <stdio.h>      // fprintf
#include <fstream>      // std::ifstream, std::ofstream
#include <system_error> // std::error_code

#include <mz.h>              // MZ_OK, MZ_COMPRESS_METHOD_DEFLATE, MZ_COMPRESS_LEVEL_DEFAULT
#include <mz_strm.h>         // mz_stream_read_cb, mz_stream_write_cb
#include <mz_zip.h>          // mz_zip_file
#include <mz_zip_rw.h>       // mz_zip_writer_*
#include <nlohmann/json.hpp> // nlohmann::json

#include "pack_icon_data.hpp"

namespace
{

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
    const std::filesystem::path& filePath,
    std::string_view             name,
    std::string_view             description,
    std::string_view             nameSpace,
    const Version&               version)
    : filePath_(filePath), name_(name), description_(description)
    , nameSpace_(nameSpace), version_(version)
{
    if (filePath_.empty())
    {
        fprintf(stderr, "MCPack::MCPack() Parameter 'filePath' is empty\n");
        return;
    }

    generateTempPath();
    if (!isOk_)
        return;
    contentPath_ = tempPath() / "content";

    if (!fs_utils::ensureDirectoryExists(getFunctionsPath()) ||
        !fs_utils::ensureDirectoryExists(getStructuresPath()))
    {
        fprintf(stderr, "MCPack::MCPack() Failed to create the functions/structures directory\n");
        isOk_ = false;
        return;
    }

    const std::filesystem::path& tickJsonPath = contentPath_ / "functions" / "tick.json";
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

std::filesystem::path MCPack::getFunctionsPath() const
{
    return contentPath_ / "functions" / fs_utils::pathFromUtf8(nameSpace_);
}

std::filesystem::path MCPack::getStructuresPath() const
{
    return contentPath_ / "structures" / fs_utils::pathFromUtf8(nameSpace_);
}

bool MCPack::addFunction(std::string_view relativePath, const std::vector<std::string>& mcfunction) const
{
    if (!isOk_ || relativePath.empty())
        return false;

    const std::filesystem::path& path =
        getFunctionsPath() / fs_utils::pathFromUtf8(std::string(relativePath) + ".mcfunction");
    if (!fs_utils::ensureDirectoryExists(path.parent_path()))
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

    for (const auto& command : mcfunction)
        file << command << '\n';
    file.close();
    return true;
}

bool MCPack::addTickFunction(std::string_view relativePath) const
{
    if (!isOk_ || relativePath.empty())
        return false;

    const std::filesystem::path& tickJsonPath = contentPath_ / "functions" / "tick.json";

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

    const std::string& entry = nameSpace_ + "/" + std::string(relativePath);
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
            {"uuid", fs_utils::generateUuid4()},
            {"version", {version_.major, version_.minor, version_.patch}},
            {"min_engine_version", {minVersion_.major, minVersion_.minor, minVersion_.patch}}
        }},
        {"modules", nlohmann::json::array({
            {
                {"description", description_},
                {"type", "data"},
                {"uuid", fs_utils::generateUuid4()},
                {"version", {version_.major, version_.minor, version_.patch}}
            }
        })}
    };

    const std::filesystem::path& manifestPath = contentPath_ / "manifest.json";
    std::ofstream manifestFile(manifestPath, std::ios::binary | std::ios::trunc);
    if (!manifestFile)
    {
        fprintf(stderr, "MCPack::pack() Failed to create 'manifest.json'\n");
        removeTempPath();
        return false;
    }
    manifestFile << manifest.dump(4);
    manifestFile.close();

    const std::filesystem::path& iconPath = contentPath_ / "pack_icon.png";
    std::ofstream iconFile(iconPath, std::ios::binary | std::ios::trunc);
    if (!iconFile)
    {
        fprintf(stderr, "MCPack::pack() Failed to create 'pack_icon.png'\n");
        removeTempPath();
        return false;
    }
    iconFile.write(reinterpret_cast<const char*>(DEFAULT_PACK_ICON_PNG), DEFAULT_PACK_ICON_PNG_SIZE);
    iconFile.close();

    const std::filesystem::path& functionsRootPath       = contentPath_ / "functions";
    const std::filesystem::path& functionsNameSpacePath  = getFunctionsPath();
    const std::filesystem::path& structuresRootPath      = contentPath_ / "structures";
    const std::filesystem::path& structuresNameSpacePath = getStructuresPath();

    std::error_code ec;
    if (isDirectoryEmptyOfFiles(functionsNameSpacePath))
        std::filesystem::remove_all(functionsRootPath, ec);
    if (isDirectoryEmptyOfFiles(structuresNameSpacePath))
        std::filesystem::remove_all(structuresRootPath, ec);

    const std::filesystem::path& zipPath     = filePath_;
    const std::filesystem::path& tempZipPath = tempPath() / zipPath.filename();

    void* writer = mz_zip_writer_create();
    if (!writer)
    {
        fprintf(stderr, "MCPack::pack() Failed to create the zip writer\n");
        removeTempPath();
        return false;
    }

    mz_zip_writer_set_compress_method(writer, MZ_COMPRESS_METHOD_DEFLATE);
    mz_zip_writer_set_compress_level(writer, MZ_COMPRESS_LEVEL_DEFAULT);

    const std::string& tempZipPathUtf8  = tempZipPath.u8string();
    const std::string& contentPathUtf8  = contentPath_.u8string();

    int32_t err = mz_zip_writer_open_file(writer, tempZipPathUtf8.c_str(), 0, 0);
    if (err != MZ_OK)
        fprintf(
            stderr, "MCPack::pack() Failed to open the zip file '%s' (error %d)\n", tempZipPath.string().c_str(), err
        );

    if (err == MZ_OK)
    {
        err = mz_zip_writer_add_path(writer, contentPathUtf8.c_str(), contentPathUtf8.c_str(), 0, 1);
        if (err != MZ_OK)
            fprintf(stderr, "MCPack::pack() Failed to add the pack contents to the zip (error %d)\n", err);
    }

    const int32_t closeErr = mz_zip_writer_close(writer);
    if (closeErr != MZ_OK)
        fprintf(stderr, "MCPack::pack() Failed to close the zip writer (error %d)\n", closeErr);

    mz_zip_writer_delete(&writer);

    bool ok = err == MZ_OK && closeErr == MZ_OK;
    if (ok)
        ok = fs_utils::moveFileReplacing(tempZipPath, zipPath);

    removeTempPath();

    return ok;
}

void MCPack::generateTempPath()
{
    tempPath_      = fs_utils::createUniqueHiddenTempDir(filePath_);
#ifdef _WIN32
    tempPathAlias_ = std::make_unique<fs_utils::AsciiSafeAlias>(tempPath_);
#endif // _WIN32
    isOk_          = !tempPath_.empty() && !tempPathAlias_->path().empty();
}

void MCPack::removeTempPath() const
{
    fs_utils::removeDirectoryRecursive(tempPath_);
}

const std::filesystem::path& MCPack::tempPath() const
{
#ifdef _WIN32
    return tempPathAlias_->path();
#else
    return tempPath_;
#endif // _WIN32
}
