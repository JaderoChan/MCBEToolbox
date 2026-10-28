#include "factory_mcpack_wrapper.hpp"

#include <stdio.h>    // fprintf
#include <exception>  // std::exception
#include <filesystem> // std::filesystem
#include <fstream>    // std::ofstream

#include <mcnbt/mcnbt.hpp> // nbt::Tag, nbt::TT_END

#include <fs_utils.hpp>
#include <image_utilities.hpp>
#include <mcpack.hpp>

namespace
{

// 便利函数：判断流是否已就绪（已打开且未到达末尾）。
bool isStreamReady(const ImageOStream& stream)
{
    return stream.isOpened() && !stream.isEnd();
}

// 便利函数：将 FactoryOptionBase 中各工厂通用的选项应用到给定工厂实例上。
template <typename Factory>
void applyBaseFactoryOptions(Factory& factory, const FactoryOptionBase& option)
{
    if (option.needUpdateBlocks) factory.setBlocks(option.blocks);
    factory.setDesiredSurface(option.desiredSurface);
    factory.setFallbackBlock(option.fallbackBlock);
    factory.setProgressCallback(option.callback);
    factory.setUserData(option.userdata);
}

// 便利函数：按 MCPackOption 配置给定的 MCPack 实例。
void configureMCPack(MCPack& pack, const MCPackOption& packOption)
{
    pack.setFormatVersion(packOption.formatVersion);
    pack.setMinVersion(packOption.minVersion);
}

} // namespace

FactoryResult generateBlockImage(
    ImageOStream& stream, BlockImageFactory& factory, const FactoryOptionBlockImage& option)
{
    if (!isStreamReady(stream))
        return {false, {}};

    applyBaseFactoryOptions(factory, option);
    factory.setTexturesDirPath(option.textureDirPath);

    const std::filesystem::path& outFilePath = option.outFilePath;
    const std::filesystem::path& tempDir     = fs_utils::createUniqueHiddenTempDir(outFilePath);
    if (tempDir.empty())
        return {false, {}};

    const std::filesystem::path& tempFilePath = tempDir / outFilePath.filename();

    bool ok = false;
    if (option.generateVideo)
    {
        auto* videoStream = dynamic_cast<VideoImageOStream*>(&stream);
        if (!videoStream)
        {
            fprintf(
                stderr,
                "generateBlockImage() option.generateVideo is true but 'stream' is not a VideoImageOStream\n"
            );
            fs_utils::removeDirectoryRecursive(tempDir);
            return {false, {}};
        }
        ok = factory.generateBlockVideo(*videoStream, tempFilePath, option.numThreads);
    }
    else
    {
        auto* singleStream = dynamic_cast<SingleImageOStream*>(&stream);
        if (!singleStream)
        {
            fprintf(
                stderr,
                "generateBlockImage() option.generateVideo is false but 'stream' is not a SingleImageOStream\n"
            );
            fs_utils::removeDirectoryRecursive(tempDir);
            return {false, {}};
        }
        const cv::Mat& image = factory.generateBlockImage(*singleStream);
        ok = imwriteUnicode(tempFilePath, image);
    }

    const auto& usage = factory.getBlockUsageMap();

    if (ok)
        ok = fs_utils::moveFileReplacing(tempFilePath, outFilePath);

    fs_utils::removeDirectoryRecursive(tempDir);

    return {ok, usage};
}

FactoryResult generateMCStructurePack(
    ImageOStream& stream, MCStructureFactory& factory, const FactoryOptionMCPack& option)
{
    if (!isStreamReady(stream))
        return {false, {}};

    applyBaseFactoryOptions(factory, option);

    MCPack pack(
        option.outFilePath, option.packOption.name, option.packOption.desciption,
        option.packOption.nameSpace, option.packOption.version);
    configureMCPack(pack, option.packOption);
    if (!pack.isOk())
        return {false, {}};

    bool ok = false;
    if (option.detachFileEnabled)
    {
        ok = factory.generateDetachMCStructure(stream, pack.getStructuresPath(), option.numThreads);
        if (option.auxFunctionEnabled)
        {
            // TODO
        }
    }
    else
    {
        const nbt::Tag& mcstructure = factory.generateSingleMCStructure(stream, true);
        ok = mcstructure.type() != nbt::TT_END;
        if (ok)
        {
            const std::filesystem::path& path =
                pack.getStructuresPath() / fs_utils::pathFromUtf8(option.packOption.name + ".mcstructure");
            std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
            if (!ofs)
            {
                fprintf(stderr, "generateMCStructurePack() Failed to create the file '%s'\n", path.string().c_str());
                ok = false;
            }
            else
            {
                try
                {
                    mcstructure.dump(ofs, false);
                }
                catch (const std::exception& e)
                {
                    fprintf(stderr, "generateMCStructurePack() Failed to dump the mcstructure: %s\n", e.what());
                    ok = false;
                }
            }

            if (option.auxFunctionEnabled)
            {
                // TODO
            }
        }
    }

    const auto& usage = factory.getBlockUsageMap();
    if (ok) ok = pack.pack();

    return {ok, usage};
}

FactoryResult generateMCFucntionPack(
    ImageOStream& stream, MCFunctionFactory& factory, const FactoryOptionMCPack& option)
{
    if (!isStreamReady(stream))
        return {false, {}};

    applyBaseFactoryOptions(factory, option);

    MCPack pack(
        option.outFilePath, option.packOption.name, option.packOption.desciption,
        option.packOption.nameSpace, option.packOption.version);
    configureMCPack(pack, option.packOption);
    if (!pack.isOk())
        return {false, {}};

    bool ok = false;
    if (option.detachFileEnabled)
    {
        ok = factory.generateDetachMCFunction(stream, pack.getFunctionsPath(), option.numThreads);
        if (option.auxFunctionEnabled)
        {
            // TODO
        }
    }
    else
    {
        const MCFunctionFactory::MCFunction& mcfunction = factory.generateSingleMCFunction(stream, true);
        ok = !mcfunction.empty() && pack.addFunction(option.packOption.name, mcfunction);
        if (option.auxFunctionEnabled)
        {
            // TODO
        }
    }

    const auto& usage = factory.getBlockUsageMap();
    if (ok) ok = pack.pack();

    return {ok, usage};
}
