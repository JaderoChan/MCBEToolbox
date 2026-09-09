#include <fstream>
#include <iostream>
#include <string>

#include <opencv2/imgcodecs.hpp>

#include <mcbe_toolbox_api.hpp>

void progressCallback(
    std::size_t    current,
    std::size_t    total,
    bool&          stop,
    void*          userdata)
{
    (void)stop;
    (void)userdata;
    std::cout << "[" << current << "/" << total << "]" << std::endl;
}

int main(int argc, char* argv[])
{
    BlockEntryMap blockEntryMap;
    // 加载并解析 BlockEntryMap
    {
        const std::string blockEntriesFilePath = "./block_entries.json";
        std::ifstream blockEntriesFile(blockEntriesFilePath);
        if (!blockEntriesFile.is_open())
        {
            std::cout << "Failed to open the file: " << blockEntriesFilePath << std::endl;
            return 1;
        }

        std::string json(
            (std::istreambuf_iterator<char>(blockEntriesFile)),
            std::istreambuf_iterator<char>()
        );

        try
        {
            blockEntryMap = parseBlockEntryMap(json);
        }
        catch (std::exception& e)
        {
            std::cout << "Failed to parse 'block_entries.json' file: " << e.what() << std::endl;
            blockEntriesFile.close();
            return 1;
        }
        blockEntriesFile.close();
    }

    // 筛选 BlockEntryMap
    auto defaultBlockDataMap = resolveBlockEntryMap(blockEntryMap);
    constexpr BlockAttributes attributes =
        BLOCK_ATTRI_HAS_PATTERN | BLOCK_ATTRI_IS_INCOMPLETE | BLOCK_ATTRI_IS_TRANSPARENT;
    auto filteredBlockDataMap = filterBlockDataMap(
        defaultBlockDataMap,
        BlockAttributeMatchMode::Disjoint,
        attributes
    );

    // 加载视频
    std::string videoFilePath;
    std::cout << "Please input the video file path:" << std::endl;
    std::cin >> videoFilePath;
    // 最多 300 帧
    VideoFramesOStream stream(videoFilePath, 300, -1, 180);
    if (!stream.isOpened())
    {
        std::cout << "Failed to open the video file: " << videoFilePath << std::endl;
        return 1;
    }

    // 转换视频为结构文件
    StructureFactory structureFactory(filteredBlockDataMap, TargetSurface::Side);
    structureFactory.setFallbackBlock(&AIR_BLOCK_DATA_PAIR);
    structureFactory.setProgressCallback(&progressCallback);

    std::cout << "=> Start Convert" << std::endl;
    nbt::Tag structure = structureFactory.generateSingleStructure(stream);
    if (structure.type() == nbt::TT_END)
    {
        std::cout << "Failed to convert image to MC Structure" << std::endl;
        return 1;
    }
    std::cout << "=> Convert finished" << std::endl;

    // 保存结果
    try
    {
        structure.dump("./out.mcstructure", false);
        std::cout << "Successfully save the MC Structure file to './out.mcstructure'" << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << "Failed to save the MC Structure file to './out.mcstructure': " << e.what() << std::endl;
    }

    std::ofstream blockUsageFile("./block_usage_count.txt");
    if (blockUsageFile.is_open())
    {
        for (const auto& [id, count] : structureFactory.getBlockUsageCount())
        {
            const std::string blockGameId = (
                id == AIR_BLOCK_DATA_PAIR.first
                ? AIR_BLOCK_DATA_PAIR.second->id
                : blockEntryMap[id].defaultBlockData.id
            );
            blockUsageFile << blockGameId << " " << count << std::endl;
        }
        std::cout << "Successfully save the block usage count result to './block_usage_count.txt'" << std::endl;
        blockUsageFile.close();
    }
    else
    {
        std::cout << "Failed to save the block usage count result to './block_usage_count.txt'" << std::endl;
    }

    return 0;
}
