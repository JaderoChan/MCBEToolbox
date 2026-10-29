#pragma once

#include <filesystem>

#include "block.hpp"
#include "block_image_factory.hpp"
#include "gif_image_ostream.hpp"
#include "image_utilities.hpp"
#include "mcfunction_factory.hpp"
#include "mcpack.hpp"
#include "mcstructure_factory.hpp"
#include "single_image_ostream.hpp"
#include "video_image_ostream.hpp"

/**
 * @file
 *
 * 为各工厂类提供简单统一的接口使之能“一键式”生成产物，
 * 此外为 #MCStructureFactory 与 #MCFunctionFactory 提供额外的 MC Pack 处理。
 */

struct FactoryOptionBase
{
    bool                          needUpdateBlocks;
    BlockDataMap                  blocks;
    SurfaceDirection              desiredSurface;
    BlockDataPair*                fallbackBlock;
    BaseFactory::ProgressCallback callback;
    void*                         userdata;
    std::filesystem::path         outFilePath;
    int                           numThreads;
};

struct FactoryResult
{
    bool ok;
    BaseFactory::BlockUsageMap usage;
};

struct FactoryOptionBlockImage : public FactoryOptionBase
{
    bool                  generateVideo;
    std::filesystem::path textureDirPath;
};

struct MCPackOption
{
    int         formatVersion;
    std::string name;
    std::string desciption;
    std::string nameSpace;
    Version     version;
    Version     minVersion;
};

struct FactoryOptionMCPack : public FactoryOptionBase
{
    bool         auxFunctionEnabled;
    bool         detachFileEnabled;
    MCPackOption packOption;
};

FactoryResult generateBlockImage(
    ImageOStream& stream, BlockImageFactory& factory, const FactoryOptionBlockImage& option);

FactoryResult generateMCStructurePack(
    ImageOStream& stream, MCStructureFactory& factory, const FactoryOptionMCPack& option);

FactoryResult generateMCFucntionPack(
    ImageOStream& stream, MCFunctionFactory& factory, const FactoryOptionMCPack& option);

