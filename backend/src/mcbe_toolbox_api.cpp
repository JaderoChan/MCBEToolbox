#include <mcbe_toolbox_api.hpp>

#include <stdio.h>    // fprintf
#include <exception>  // std::exception
#include <filesystem> // std::filesystem
#include <fstream>    // std::ofstream
#include <memory>     // std::unique_ptr

#include <mcnbt/mcnbt.hpp> // nbt::Tag, nbt::TT_END

#include <fs_utils.hpp>
#include <image_utilities.hpp>
#include <mcpack.hpp>
#include "assist_function_generator.hpp"

namespace
{

// 便利函数：判断流是否已就绪（已打开且未到达末尾）。
bool isStreamReady(const ImageOStream& stream)
{
    return stream.isOpened() && !stream.isEnd();
}

// 便利函数：将 GeneratorCommonOption 中各工厂通用的选项应用到给定工厂实例上。
template <typename Factory>
void applyBaseFactoryOptions(Factory& factory, const GeneratorCommonOption& option)
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
    ImageOStream& stream, BlockImageFactory& factory, const BlockImageGeneratorOption& option)
{
    if (!isStreamReady(stream))
        return {BaseFactory::FAIL, {}, "The input stream is not opened or has arrived at the end"};

    applyBaseFactoryOptions(factory, option);
    factory.setTexturesDirPath(option.textureDirPath);

    const std::filesystem::path& outFilePath = option.outFilePath;
    const std::filesystem::path& tempDir     = fs_utils::createUniqueHiddenTempDir(outFilePath);
    if (tempDir.empty())
        return {BaseFactory::FAIL, {}, "Failed to create a temporary working directory"};
#ifdef _WIN32
    std::unique_ptr<fs_utils::AsciiSafeAlias> tempDirAlias = std::make_unique<fs_utils::AsciiSafeAlias>(tempDir);
    if (tempDirAlias->path().empty())
        return {BaseFactory::FAIL, {}, "Failed to create an ASCII-safe alias for the temporary working directory"};
    const std::filesystem::path& tempDirRef = tempDirAlias->path();
#else
    const std::filesystem::path& tempDirRef = tempDir;
#endif // _WIN32
    const fs_utils::ScopedPathRemover tempDirRemover(tempDir);

    const std::filesystem::path& tempFilePath = tempDirRef / outFilePath.filename();

    BaseFactory::ResultType result = BaseFactory::FAIL;
    std::string             message;
    if (option.generateVideo)
    {
        auto* videoStream = dynamic_cast<VideoImageOStream*>(&stream);
        if (!videoStream)
        {
            message = "option.generateVideo is true but 'stream' is not a VideoImageOStream";
            fprintf(stderr, "generateBlockImage() %s\n", message.c_str());
            return {BaseFactory::FAIL, {}, message};
        }
        result = factory.generateBlockVideo(*videoStream, tempFilePath, option.numThreads);
        if (result != BaseFactory::SUCCESS)
            message = factory.getErrorMessage();
    }
    else
    {
        auto* singleStream = dynamic_cast<SingleImageOStream*>(&stream);
        if (!singleStream)
        {
            message = "option.generateVideo is false but 'stream' is not a SingleImageOStream";
            fprintf(stderr, "generateBlockImage() %s\n", message.c_str());
            return {BaseFactory::FAIL, {}, message};
        }
        cv::Mat image;
        result = factory.generateBlockImage(*singleStream, image);
        if (result != BaseFactory::SUCCESS)
        {
            message = factory.getErrorMessage();
        }
        else if (!imwriteUnicode(tempFilePath, image))
        {
            result  = BaseFactory::FAIL;
            message = "Failed to write the generated block image to the temporary file";
        }
    }

    const auto& usage = factory.getBlockUsageMap();

    if (result == BaseFactory::SUCCESS && !fs_utils::moveFileReplacing(tempFilePath, outFilePath))
    {
        result  = BaseFactory::FAIL;
        message = "Failed to move the generated file to the output path";
    }

    return {result, usage, message};
}

FactoryResult generateStructurePack(
    ImageOStream& stream, MCStructureFactory& factory, const MCPackGeneratorOption& option)
{
    if (!isStreamReady(stream))
        return {BaseFactory::FAIL, {}, "The input stream is not opened or has arrived at the end"};

    applyBaseFactoryOptions(factory, option);

    MCPack pack(
        option.outFilePath, option.packOption.name, option.packOption.desciption,
        option.packOption.nameSpace, option.packOption.version);
    configureMCPack(pack, option.packOption);
    if (!pack.isOk())
        return {BaseFactory::FAIL, {}, "Failed to prepare the temporary MC Pack working directory"};

    const auto frameSize = stream.frameSize();

    BaseFactory::ResultType result = BaseFactory::FAIL;
    std::string             message;
    if (option.detachFileEnabled)
    {
        result = factory.generateDetachMCStructure(stream, pack.getStructuresPath(), pack.getName(), option.numThreads);
        if (result != BaseFactory::SUCCESS)
            message = factory.getErrorMessage();

        if (result == BaseFactory::SUCCESS && option.assistFunctionEnabled)
        {
            // TODO
        }
    }
    else
    {
        nbt::Tag mcstructure;
        result = factory.generateSingleMCStructure(stream, true, mcstructure);
        if (result != BaseFactory::SUCCESS)
        {
            message = factory.getErrorMessage();
        }
        else
        {
            const std::filesystem::path& path =
                pack.getStructuresPath() / fs_utils::pathFromUtf8(option.packOption.name.u8string() + ".mcstructure");
            std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
            if (!ofs)
            {
                message = "Failed to create the file '" + path.u8string() + "'";
                fprintf(stderr, "generateStructurePack() %s\n", message.c_str());
                result = BaseFactory::FAIL;
            }
            else
            {
                try
                {
                    mcstructure.dump(ofs, false);
                }
                catch (const std::exception& e)
                {
                    message = std::string("Failed to dump the mcstructure: ") + e.what();
                    fprintf(stderr, "generateStructurePack() %s\n", message.c_str());
                    result = BaseFactory::FAIL;
                }
            }

            if (result == BaseFactory::SUCCESS && option.assistFunctionEnabled)
            {
                const std::string structureName = option.packOption.nameSpace + ":" + option.packOption.name.u8string();
                AssistFunctionGenerator funcGen(
                    frameSize.width, frameSize.height, option.desiredSurface, structureName);
                pack.addFunction("setOrigin",        funcGen.makeSetOriginFunction());
                pack.addFunction("clearOrigin",      funcGen.makeClearOriginFunction());
                pack.addFunction("build",            funcGen.makeBuildFunction());
                pack.addFunction("moveToWatchPoint", funcGen.makeMoveToWatchPointFunction());
            }
        }
    }

    const auto& usage = factory.getBlockUsageMap();
    if (result == BaseFactory::SUCCESS && !pack.pack())
    {
        result  = BaseFactory::FAIL;
        message = "Failed to pack the generated contents into the .mcpack file";
    }

    return {result, usage, message};
}

FactoryResult generateFucntionPack(
    ImageOStream& stream, MCFunctionFactory& factory, const MCPackGeneratorOption& option)
{
    if (!isStreamReady(stream))
        return {BaseFactory::FAIL, {}, "The input stream is not opened or has arrived at the end"};

    applyBaseFactoryOptions(factory, option);

    MCPack pack(
        option.outFilePath, option.packOption.name, option.packOption.desciption,
        option.packOption.nameSpace, option.packOption.version);
    configureMCPack(pack, option.packOption);
    if (!pack.isOk())
        return {BaseFactory::FAIL, {}, "Failed to prepare the temporary MC Pack working directory"};

    BaseFactory::ResultType result = BaseFactory::FAIL;
    std::string             message;
    if (option.detachFileEnabled)
    {
        result = factory.generateDetachMCFunction(stream, pack.getFunctionsPath(), pack.getName(), option.numThreads);
        if (result != BaseFactory::SUCCESS)
            message = factory.getErrorMessage();

        if (result == BaseFactory::SUCCESS && option.assistFunctionEnabled)
        {
            // TODO
        }
    }
    else
    {
        MCFunctionFactory::MCFunction mcfunction;
        result = factory.generateSingleMCFunction(stream, true, mcfunction);
        // TODO: 需要做 mcfunction 文件函数最大数量限制。
        if (result != BaseFactory::SUCCESS)
        {
            message = factory.getErrorMessage();
        }
        else if (!pack.addFunction(option.packOption.name.u8string(), mcfunction))
        {
            result  = BaseFactory::FAIL;
            message = "Failed to add the generated mcfunction to the pack";
        }
        if (option.assistFunctionEnabled)
        {
            // TODO
        }
    }

    const auto& usage = factory.getBlockUsageMap();
    if (result == BaseFactory::SUCCESS && !pack.pack())
    {
        result  = BaseFactory::FAIL;
        message = "Failed to pack the generated contents into the .mcpack file";
    }

    return {result, usage, message};
}
