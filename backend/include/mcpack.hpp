#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "version.hpp"

class MCPack
{
public:
    MCPack(
        std::string_view filePath,
        std::string_view name,
        std::string_view description,
        std::string_view _namespace,
        const Version&   version = Version(1, 0, 0));
    ~MCPack();

    MCPack(const MCPack&)            = delete;
    MCPack& operator=(const MCPack&) = delete;

    void setFormatVersion(int version)         { formatVersion_ = version; }
    void setVersion(const Version& version)    { version_       = version; }
    void setMinVersion(const Version& version) { minVersion_    = version; }

    int         getFormatVersion() const { return formatVersion_; }
    std::string getFilePath()      const { return filePath_;      }
    std::string getName()          const { return name_;          }
    std::string getDescription()   const { return description_;   }
    std::string getNamespace()     const { return namespace_;     }
    Version     getVersion()       const { return version_;       }
    Version     getMinVersion()    const { return minVersion_;    }

    /** 获取临时工作目录下 "functions/<namespace>" 子目录路径，用于存放 .mcfunction 文件。 */
    std::string getFunctionsPath()  const;
    /** 获取临时工作目录下 "structures/<namespace>" 子目录路径，用于存放 .mcstructure 文件。 */
    std::string getStructuresPath() const;

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

    /** 打包：生成 manifest.json 与 pack_icon.png，将临时目录压缩为 .mcpack 文件，完成后清理临时目录。 */
    bool pack() const;

private:
    static std::string generateUuid();

    std::string generateTempPath();
    void        removeTempPath() const;

    int         formatVersion_ = 2;
    std::string filePath_;
    std::string name_;
    std::string description_;
    std::string namespace_;
    Version     version_;
    Version     minVersion_ = Version(1, 21, 50);

    std::string tempPath_;
    bool        isOk_ = false;
};
