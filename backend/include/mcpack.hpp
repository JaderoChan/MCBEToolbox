#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "fs_utils.hpp"
#include "version.hpp"

class MCPack
{
public:
    MCPack(
        std::filesystem::path filePath,
        std::filesystem::path name,
        std::string_view      description,
        std::string_view      nameSpace,
        Version               version = Version(1, 0, 0));
    ~MCPack();

    MCPack(const MCPack&)            = delete;
    MCPack& operator=(const MCPack&) = delete;

    void setFormatVersion(int version)  { formatVersion_ = version;            }
    void setVersion(Version version)    { version_       = std::move(version); }
    void setMinVersion(Version version) { minVersion_    = std::move(version); }

    int                          getFormatVersion() const { return formatVersion_; }
    const std::filesystem::path& getFilePath()      const { return filePath_;      }
    const std::filesystem::path& getName()          const { return name_;          }
    const std::string&           getDescription()   const { return description_;   }
    const std::string&           getNameSpace()     const { return nameSpace_;     }
    const Version&               getVersion()       const { return version_;       }
    const Version&               getMinVersion()    const { return minVersion_;    }

    /** 获取临时工作目录下 "functions/<namespace>" 子目录路径，用于存放 .mcfunction 文件。 */
    std::filesystem::path getFunctionsPath()  const;
    /** 获取临时工作目录下 "structures/<namespace>" 子目录路径，用于存放 .mcstructure 文件。 */
    std::filesystem::path getStructuresPath() const;

    /**
     * 添加一个 .mcfunction 文件到 getFunctionsPath() 目录下。
     *
     * @param relativePath 相对于 getFunctionsPath() 的路径（不含 ".mcfunction" 后缀），可包含子目录，
     *                     子目录不存在时会自动创建。
     */
    bool addFunction(std::string_view relativePath, const std::vector<std::string>& mcfunction) const;

    /**
     * 将指定函数加入 functions/tick.json，使其每 tick 自动执行一次（已存在则忽略）。
     *
     * @param relativePath 与 addFunction() 一致的相对路径（不含 ".mcfunction" 后缀）。
     */
    bool addTickFunction(std::string_view relativePath) const;

    /** 临时工作目录是否已就绪）。 */
    bool isOk() const { return isOk_; }

    /**
     * 打包：生成 manifest.json 与 pack_icon.png，将临时工作目录下的内容压缩为一个临时 zip 文件，
     * 压缩成功后再将其移动至 #filePath 指定的位置，无论成功与否都会清理临时目录。
     */
    bool pack() const;

private:
    void generateTempPath();
    void removeTempPath() const;
    const std::filesystem::path& tempPath() const;

    int                   formatVersion_ = 2;
    std::filesystem::path filePath_;
    std::filesystem::path name_;
    std::string           description_;
    std::string           nameSpace_;
    Version               version_;
    Version               minVersion_ = Version(1, 21, 50);

    // 临时工作目录。
    std::filesystem::path tempPath_;
#ifdef _WIN32
    // 临时工作目录别名。
    std::unique_ptr<fs_utils::AsciiSafeAlias> tempPathAlias_;
#endif // _WIN32
    // 临时工作目录下实际存放 mcpack 内容（functions/structures/manifest.json/pack_icon.png）的子目录，
    // 与压缩产物（直接位于 #tempPath_ 下）相互隔离。
    std::filesystem::path contentPath_;
    bool isOk_ = false;
};
