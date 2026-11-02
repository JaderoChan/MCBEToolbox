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

struct GeneratorCommonOption
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

struct BlockImageGeneratorOption : public GeneratorCommonOption
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

struct MCPackGeneratorOption : public GeneratorCommonOption
{
    bool         auxFunctionEnabled;
    bool         detachFileEnabled;
    MCPackOption packOption;
};

/** @throw cv::Exception OpenCV 内部读写图像/视频时在特殊情况下可能抛出异常（例如目标扩展名无可用编码器）。 */
FactoryResult generateBlockImage(
    ImageOStream& stream, BlockImageFactory& factory, const BlockImageGeneratorOption& option);

/**
 * @throw cv::Exception OpenCV 内部读写图像/视频时在特殊情况下可能抛出异常（例如目标扩展名无可用编码器）。
 * @throw std::exception NBT 序列化失败时可能抛出异常。
 */
FactoryResult generateMCStructurePack(
    ImageOStream& stream, MCStructureFactory& factory, const MCPackGeneratorOption& option);

/** @throw cv::Exception OpenCV 内部读写图像/视频时在特殊情况下可能抛出异常（例如目标扩展名无可用编码器）。 */
FactoryResult generateMCFucntionPack(
    ImageOStream& stream, MCFunctionFactory& factory, const MCPackGeneratorOption& option);
