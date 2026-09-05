#include <fstream>
#include <iostream>
#include <string>

#include <mcbe_toolbox_api.hpp>

void progressCallback(std::size_t current, std::size_t total, bool& stop, void* userdata)
{
    std::cout << "[" << current << "/" << total << "]" << std::endl;
}

int main(int argc, char* argv[])
{
    BlockEntryMap blockEntryMap;
    // 加载并解析 BlockEntryMap
    {
        std::string blockEntriesFilepath;
        std::cout << "Please input the 'block_entries.json' file path:" << std::endl;
        std::cin >> blockEntriesFilepath;

        std::ifstream blockEntriesFile(blockEntriesFilepath);
        if (!blockEntriesFile.is_open())
        {
            std::cout << "Failed to open the file: " << blockEntriesFilepath << std::endl;
            return 1;
        }

        std::string json(
            (std::istreambuf_iterator<char>(blockEntriesFile)),
            std::istreambuf_iterator<char>()
        );

        try
        {
            blockEntryMap = parseBlockEntryMapFromJson(json);
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
    auto filteredBlockDataMap = filterBlockAttributes(
        defaultBlockDataMap,
        BlockAttributeMatchMode::Disjoint,
        attributes
    );

    // 加载图片
    cv::Mat image;
    {
        std::string imageFilepath;
        std::cout << "Please input the image file path:" << std::endl;
        std::cin >> imageFilepath;

        image = cv::imread(imageFilepath, cv::IMREAD_UNCHANGED);
        if (image.empty())
        {
            std::cout << "Failed to load the image: " << imageFilepath << std::endl;
            releaseBlockEntryMap(blockEntryMap);
            return 1;
        }

        // 将图像限制在一定尺寸内
        if (image.rows > 1080 || image.cols > 1080)
        {
            const double ratio = 1080.0 / std::max(image.rows, image.cols);
            cv::resize(image, image, cv::Size(0, 0), ratio, ratio);
        }
    }

    // 转换图像
    std::map<std::string, std::size_t> blockUsageCount;
    std::cout << "=> Start Convert" << std::endl;
    cv::Mat result = convertImageToBlockImage(
        image,
        filteredBlockDataMap,
        TargetSurface::Side,
        nullptr,
        &blockUsageCount,
        &progressCallback
    );
    if (result.empty())
    {
        std::cout << "Failed to convert image to block image" << std::endl;
        releaseBlockEntryMap(blockEntryMap);
        return 1;
    }
    std::cout << "=> Convert finished" << std::endl;

    // 保存结果
    if (cv::imwrite("./out.png", result))
        std::cout << "Successfully save the result image to './out.png'" << std::endl;
    else
        std::cout << "Failed to save the result image to './out.png'" << std::endl;

    std::ofstream blockUsageFile("./block_usage_count.txt");
    if (blockUsageFile.is_open())
    {
        for (const auto& [id, count] : blockUsageCount)
            blockUsageFile << blockEntryMap[id]->name << " " << count << std::endl;
        std::cout << "Successfully save the block usage count result to './block_usage_count.txt'" << std::endl;
        blockUsageFile.close();
    }
    else
    {
        std::cout << "Failed to save the block usage count result to './block_usage_count.txt'" << std::endl;
    }

    releaseBlockEntryMap(blockEntryMap);
    return 0;
}
