#include <filesystem> // std::filesystem::*
#include <iostream>   // std::cin, std::cout
#include <string>     // std::string, std::to_string

#include <opencv2/imgcodecs.hpp> // cv::imwrite
#include <mcbe_toolbox_api.hpp>

#include "command_line_menu.hpp"

constexpr BlockAttributes DEFAULT_FILTER_BLOCK_ATTRIBUTES =
    BLOCK_ATTRI_HAS_PATTERN   |
    BLOCK_ATTRI_IS_INCOMPLETE |
    BLOCK_ATTRI_IS_TRANSPARENT;
constexpr BlockAttributeFilterMode DEFAULT_FILTER_MODE = BLOCK_ATTRI_FILTER_MODE_DISJOINT;

struct Configurations
{
    BlockEntryMap            blockEntries;
    BlockAttributes          filterAttributes = DEFAULT_FILTER_BLOCK_ATTRIBUTES;
    BlockAttributeFilterMode filterMode       = DEFAULT_FILTER_MODE;
    BlockDataMap             blocks;
    const BlockDataPair*     fallbackBlock    = nullptr;
    cv::Size                 frameMaxSize     = cv::Size(1080, 1080);
    int                      frameMaxCount    = 600;
};

// 全局配置
static Configurations config;

// 加载 BlockEntryMap，并按照默认参数处理得到 BlockDataMap
bool loadBlockEntries()
{
    constexpr const char* BLOCK_ENTRIES_FILEPATH = "./block_entries.json";
    try
    {
        config.blockEntries = parseBlockEntriesFromFile(BLOCK_ENTRIES_FILEPATH);
        config.blocks       = resolveBlockEntries(config.blockEntries);
        config.blocks       = filterBlocks(config.blocks, config.filterMode, config.filterAttributes);
        return true;
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to parse block entries from file: " << e.what() << std::endl;
        return false;
    }
}

// 进度回调函数
void progressCllback(std::size_t current, std::size_t total, bool& stop, void* userdata)
{
    const std::size_t step = *static_cast<std::size_t*>(userdata);
    // 按照指定步长打印日志
    if (current % step == 0 || current == total)
        std::cout << "[" << current << "/" << total << "]\n";
}

// 通过 CLI 获取期望面
SurfaceDirection getDesiredSurface()
{
    CommandLineMenu menu;
    menu.setTopText("Please select the desired surface");

    SurfaceDirection desiredSurface = SURFACE_DIRECTION_NORTH;
    menu.addOption("Up",     [&]() { desiredSurface = SURFACE_DIRECTION_UP;     menu.endReceiveInput(); }, false, false);
    menu.addOption("Bottom", [&]() { desiredSurface = SURFACE_DIRECTION_BOTTOM; menu.endReceiveInput(); }, false, false);
    menu.addOption("North",  [&]() { desiredSurface = SURFACE_DIRECTION_NORTH;  menu.endReceiveInput(); }, false, false);
    menu.addOption("South",  [&]() { desiredSurface = SURFACE_DIRECTION_SOUTH;  menu.endReceiveInput(); }, false, false);
    menu.addOption("East",   [&]() { desiredSurface = SURFACE_DIRECTION_EAST;   menu.endReceiveInput(); }, false, false);
    menu.addOption("West",   [&]() { desiredSurface = SURFACE_DIRECTION_WEST;   menu.endReceiveInput(); }, false, false);

    menu.show();
    menu.startReceiveInput();
    menu.clearConsole();

    return desiredSurface;
}

void generateBlockImageTriggered()
{
    BlockImageFactory factory(config.blocks, getDesiredSurface());
    factory.setFallbackBlock(config.fallbackBlock);
    factory.setProgressCallback(&progressCllback);
    std::size_t callbackStep = 1000; // 每 1000 个像素打印一次日志
    factory.setUserData(static_cast<void*>(&callbackStep));

    // 输入图像
    std::string filepath;
    std::cout << "Please input the image filepath: " << std::endl;
    std::cin >> filepath;
    ImageFramesOStream stream(filepath, config.frameMaxSize.width, config.frameMaxSize.height);
    if (!stream.isOpened())
    {
        std::cerr << "Failed open the image: " << filepath << std::endl;
        return;
    }

    // 生成方块图
    std::cout << "Start generate block image" << std::endl;
    const cv::Mat image = factory.generateBlockImage(stream);
    if (image.empty())
    {
        std::cerr << "Failed to generate the block image" << std::endl;
        return;
    }
    else
    {
        std::cout << "Block image generate finished" << std::endl;
    }

    // 保存结果
    if (cv::imwrite("./out.png", image))
        std::cout << "Successfully save the block image to './out.png'" << std::endl;
    else
        std::cerr << "Failed to save the block image to './out.png'" << std::endl;
}

void generateBlockVideoTriggered()
{
    BlockImageFactory factory(config.blocks, getDesiredSurface());
    factory.setFallbackBlock(config.fallbackBlock);
    factory.setProgressCallback(&progressCllback);
    std::size_t callbackStep = 1;
    factory.setUserData(static_cast<void*>(&callbackStep));

    // 输入视频
    std::string filepath;
    std::cout << "Please input the video filepath: " << std::endl;
    std::cin >> filepath;
    VideoFramesOStream stream(filepath, -1, config.frameMaxSize.width, config.frameMaxSize.height);
    if (!stream.isOpened())
    {
        std::cerr << "Failed open the video: " << filepath << std::endl;
        return;
    }

    // 生成方块视频
    std::cout << "Start generate block video" << std::endl;
    const bool ok = factory.generateBlockVideo(stream, "./out.mp4");
    if (!ok)
    {
        std::cerr << "Failed to generate the block video" << std::endl;
        return;
    }
    else
    {
        std::cout << "Block video generate finished" << std::endl;
    }

    std::cout << "Successfully save the block video to './out.mp4'" << std::endl;
}

void generateImageStructureTriggered()
{
    MCStructureFactory factory(config.blocks, getDesiredSurface());
    factory.setFallbackBlock(config.fallbackBlock);
    factory.setProgressCallback(progressCllback);
    std::size_t callbackStep = 1000;
    factory.setUserData(static_cast<void*>(&callbackStep));

    // 输入图像
    std::string filepath;
    std::cout << "Please input the image filepath: " << std::endl;
    std::cin >> filepath;
    ImageFramesOStream stream(filepath, config.frameMaxSize.width, config.frameMaxSize.height);
    if (!stream.isOpened())
    {
        std::cerr << "Failed open the image: " << filepath << std::endl;
        return;
    }

    // 生成 MC Structure
    std::cout << "Start generate MC Structure" << std::endl;
    const nbt::Tag mcstructure = factory.generateSingleMCStructure(stream, false);
    if (mcstructure.type() == nbt::TT_END)
    {
        std::cerr << "Failed to generate the MC Structure" << std::endl;
        return;
    }
    else
    {
        std::cout << "MC Structure generate finished" << std::endl;
    }

    const std::string name = std::filesystem::path(filepath).stem().string();
    MCPack pack("./", name, "Generated by MCBEToolbox", name);
    if (!pack.isOk())
    {
        std::cerr << "Failed to prepare the mcpack working directory" << std::endl;
        return;
    }

    mcstructure.dump(pack.getStructuresPath() + "/" + name + ".mcstructure", false);
    if (!pack.pack())
    {
        std::cerr << "Failed to pack the mcpack" << std::endl;
        return;
    }
    std::cout << "Successfully save the MC Structure pack to './" << name << ".mcpack'" << std::endl;
}

void generateVideoStructureTriggered()
{
    MCStructureFactory factory(config.blocks, getDesiredSurface());
    factory.setFallbackBlock(config.fallbackBlock);
    factory.setProgressCallback(progressCllback);
    std::size_t callbackStep = 1;
    factory.setUserData(static_cast<void*>(&callbackStep));

    bool asDetach = true;
    std::string input;
    std::cout << "Generate MC Structure as detach file? (Y/N)" << std::endl;
    std::cin >> input;
    if (input == "Y" || input == "y")
    {
        asDetach = true;
    }
    else if (input == "N" || input == "n")
    {
        asDetach = false;
    }
    else
    {
        std::cerr << "Invalid input" << std::endl;
        return;
    }

    // 输入视频
    std::string filepath;
    std::cout << "Please input the video filepath: " << std::endl;
    std::cin >> filepath;
    VideoFramesOStream stream(filepath, config.frameMaxCount, config.frameMaxSize.width, config.frameMaxSize.height);
    if (!stream.isOpened())
    {
        std::cerr << "Failed open the video: " << filepath << std::endl;
        return;
    }

    const std::string name = std::filesystem::path(filepath).stem().string();
    MCPack pack("./", name, "Generated by MCBEToolbox", name);
    if (!pack.isOk())
    {
        std::cerr << "Failed to prepare the mcpack working directory" << std::endl;
        return;
    }

    // 生成 MC Structure
    std::cout << "Start generate MC Structure" << std::endl;
    bool ok = false;
    if (asDetach)
    {
        ok = factory.generateDetachMCStructure(stream, pack.getStructuresPath());
        if (ok)
            std::cout << "MC Structure generate finished" << std::endl;
        else
            std::cerr << "Failed to generate the MC Structure" << std::endl;
    }
    else
    {
        const nbt::Tag mcstructure = factory.generateSingleMCStructure(stream, true);
        ok = mcstructure.type() != nbt::TT_END;
        if (!ok)
        {
            std::cerr << "Failed to generate the MC Structure" << std::endl;
        }
        else
        {
            std::cout << "MC Structure generate finished" << std::endl;
            mcstructure.dump(pack.getStructuresPath() + "/" + name + ".mcstructure", false);
        }
    }

    if (!ok)
        return;

    if (!pack.pack())
    {
        std::cerr << "Failed to pack the mcpack" << std::endl;
        return;
    }
    std::cout << "Successfully save the MC Structure pack to './" << name << ".mcpack'" << std::endl;
}

void generateImageFunctionTriggered()
{
    MCFunctionFactory factory(config.blocks, getDesiredSurface());
    factory.setFallbackBlock(config.fallbackBlock);
    factory.setProgressCallback(progressCllback);
    std::size_t callbackStep = 1000;
    factory.setUserData(static_cast<void*>(&callbackStep));

    // 输入图像
    std::string filepath;
    std::cout << "Please input the image filepath: " << std::endl;
    std::cin >> filepath;
    ImageFramesOStream stream(filepath, config.frameMaxSize.width, config.frameMaxSize.height);
    if (!stream.isOpened())
    {
        std::cerr << "Failed open the image: " << filepath << std::endl;
        return;
    }

    // 生成 MC Function
    std::cout << "Start generate MC Function" << std::endl;
    const auto mcfunction = factory.generateSingleMCFunction(stream, false);
    if (mcfunction.empty())
    {
        std::cerr << "Failed to generate the MC Function" << std::endl;
        return;
    }
    else
    {
        std::cout << "MC Function generate finished" << std::endl;
    }

    const std::string name = std::filesystem::path(filepath).stem().string();
    MCPack pack("./", name, "Generated by MCBEToolbox", name);
    if (!pack.isOk() || !pack.addFunction(name, mcfunction) || !pack.pack())
    {
        std::cerr << "Failed to pack the mcpack" << std::endl;
        return;
    }
    std::cout << "Successfully save the MC Function pack to './" << name << ".mcpack'" << std::endl;
}

void generateVideoFunctionTriggered()
{
    MCFunctionFactory factory(config.blocks, getDesiredSurface());
    factory.setFallbackBlock(config.fallbackBlock);
    factory.setProgressCallback(progressCllback);
    std::size_t callbackStep = 1;
    factory.setUserData(static_cast<void*>(&callbackStep));

    bool asDetach = true;
    std::string input;
    std::cout << "Generate MC Structure as detach file? (Y/N)" << std::endl;
    std::cin >> input;
    if (input == "Y" || input == "y")
    {
        asDetach = true;
    }
    else if (input == "N" || input == "n")
    {
        asDetach = false;
    }
    else
    {
        std::cerr << "Invalid input" << std::endl;
        return;
    }

    // 输入视频
    std::string filepath;
    std::cout << "Please input the video filepath: " << std::endl;
    std::cin >> filepath;
    VideoFramesOStream stream(filepath, config.frameMaxCount, config.frameMaxSize.width, config.frameMaxSize.height);
    if (!stream.isOpened())
    {
        std::cerr << "Failed open the video: " << filepath << std::endl;
        return;
    }

    const std::string name = std::filesystem::path(filepath).stem().string();
    MCPack pack("./", name, "Generated by MCBEToolbox", name);
    if (!pack.isOk())
    {
        std::cerr << "Failed to prepare the mcpack working directory" << std::endl;
        return;
    }

    // 生成 MC Function
    std::cout << "Start generate MC Function" << std::endl;
    bool ok = false;
    if (asDetach)
    {
        ok = factory.generateDetachMCFunction(stream, pack.getFunctionsPath());
        if (ok)
            std::cout << "MC Functions generate finished" << std::endl;
        else
            std::cerr << "Failed to generate the MC Functions" << std::endl;
    }
    else
    {
        const auto mcfunction = factory.generateSingleMCFunction(stream, true);
        ok = !mcfunction.empty();
        if (!ok)
        {
            std::cerr << "Failed to generate the MC Function" << std::endl;
        }
        else
        {
            std::cout << "MC Function generate finished" << std::endl;
            ok = pack.addFunction(name, mcfunction);
        }
    }

    if (!ok)
        return;

    if (!pack.pack())
    {
        std::cerr << "Failed to pack the mcpack" << std::endl;
        return;
    }
    std::cout << "Successfully save the MC Function pack to './" << name << ".mcpack'" << std::endl;
}

void filterBlocksTriggered()
{
#define TOGGLE_BLOCK_ATTRIBUTE(attri)                                           \
do {                                                                            \
    if (HAS_BLOCK_ATTRIS(config.filterAttributes, attri))                       \
        UNSET_BLOCK_ATTRIS(config.filterAttributes, attri);                     \
    else                                                                        \
        SET_BLOCK_ATTRIS(config.filterAttributes, attri);                       \
} while (0);

#define CREATE_OPTION_DISPLAY_TEXT(attri) \
(std::string(#attri) + (HAS_BLOCK_ATTRIS(config.filterAttributes, attri) ? " *" : ""))

#define APPEND_MENU_OPTION(index, attri)                                        \
menu.addOption(                                                                 \
    CREATE_OPTION_DISPLAY_TEXT(attri),                                          \
    [&]()                                                                       \
    {                                                                           \
        TOGGLE_BLOCK_ATTRIBUTE(attri);                                          \
        menu.setOptionText(index, CREATE_OPTION_DISPLAY_TEXT(attri));           \
    }, false, false)

    CommandLineMenu menu;
    menu.setTopText(
        "Disjoint it by selecting the block attribute, current blocks count: " +
        std::to_string(config.blocks.size()));

    APPEND_MENU_OPTION(0, BLOCK_ATTRI_IS_INCOMPLETE);
    APPEND_MENU_OPTION(1, BLOCK_ATTRI_IS_TRANSPARENT);
    APPEND_MENU_OPTION(2, BLOCK_ATTRI_IS_LUMINOUS);
    APPEND_MENU_OPTION(3, BLOCK_ATTRI_IS_UNSTABLE);
    APPEND_MENU_OPTION(4, BLOCK_ATTRI_IS_CREATIVE);
    APPEND_MENU_OPTION(5, BLOCK_ATTRI_HAS_GRAVITY);
    APPEND_MENU_OPTION(6, BLOCK_ATTRI_HAS_PATTERN);
    APPEND_MENU_OPTION(7, BLOCK_ATTRI_FLAMMABLE);
    APPEND_MENU_OPTION(8, BLOCK_ATTRI_ENDERMAN_PICKABLE);

    menu.addOption("Confirm", [&]()
    {
        auto blocks = resolveBlockEntries(config.blockEntries);
        config.blocks = filterBlocks(blocks, config.filterMode, config.filterAttributes);
        std::cout
            << "Successfully update blocks, current blocks count: "
            << config.blocks.size()
            << ". (press any key to return)"
            << std::endl;
        menu.endReceiveInput();
    }, false, true);

    menu.show();
    menu.startReceiveInput();

#undef APPEND_MENU_OPTION
#undef CREATE_OPTION_DISPLAY_TEXT
#undef TOGGLE_BLOCK_ATTRIBUTE
}

void selectFallbackBlockTriggered()
{
#define CREATE_MENU_TOP_TEXT \
(std::string("Current fallback block: ") + (config.fallbackBlock ? "Air" : "Empty"))

#define UPDATE_MENU_TOP_TEXT \
menu.setTopText(CREATE_MENU_TOP_TEXT)

    CommandLineMenu menu;
    UPDATE_MENU_TOP_TEXT;

    menu.addOption("Empty", [&]() { config.fallbackBlock = nullptr;              UPDATE_MENU_TOP_TEXT; }, false, false);
    menu.addOption("Air",   [&]() { config.fallbackBlock = &AIR_BLOCK_DATA_PAIR; UPDATE_MENU_TOP_TEXT; }, false, false);
    menu.addOption("Confirm", [&]()
    {
        std::cout << "Successfully update fallback block. (press any key to return)" << std::endl;
        menu.endReceiveInput();
    }, false, true);

    menu.show();
    menu.startReceiveInput();

#undef UPDATE_MENU_TOP_TEXT
#undef CREATE_MENU_TOP_TEXT
}

void limitFrameSizeTriggered()
{
#define CREATE_MENU_TOP_TEXT                                                    \
(std::string("Current frame max size: [")   +                                   \
 std::to_string(config.frameMaxSize.width)  + "*" +                             \
 std::to_string(config.frameMaxSize.height) + "]")

#define UPDATE_MENU_TOP_TEXT \
menu.setTopText(CREATE_MENU_TOP_TEXT)

    CommandLineMenu menu;
    UPDATE_MENU_TOP_TEXT;

    menu.addOption("Set max width", [&]()
    {
        std::cout << "Please input the max width:" << std::endl;
        int maxWidth = 0;
        std::cin >> maxWidth;
        config.frameMaxSize.width = maxWidth;
        UPDATE_MENU_TOP_TEXT;
    }, false, false);

    menu.addOption("Set max height", [&]()
    {
        std::cout << "Please input the max height:" << std::endl;
        int maxHeight = 0;
        std::cin >> maxHeight;
        config.frameMaxSize.height = maxHeight;
        UPDATE_MENU_TOP_TEXT;
    }, false, false);

    menu.addOption("Confirm", [&]()
    {
        std::cout << "Successfully update frame max size. (press any key to return)" << std::endl;
        menu.endReceiveInput();
    }, false, true);

    menu.show();
    menu.startReceiveInput();

#undef UPDATE_MENU_TOP_TEXT
#undef CREATE_MENU_TOP_TEXT
}

void limitFrameCountTriggered()
{
#define CREATE_MENU_TOP_TEXT \
(std::string("Current frame max count: ") + std::to_string(config.frameMaxCount))

#define UPDATE_MENU_TOP_TEXT \
menu.setTopText(CREATE_MENU_TOP_TEXT)

    CommandLineMenu menu;
    UPDATE_MENU_TOP_TEXT;

    menu.addOption("Set max count", [&]()
    {
        std::cout << "Please input the max count:" << std::endl;
        int maxCount = 0;
        std::cin >> maxCount;
        config.frameMaxCount = maxCount;
        UPDATE_MENU_TOP_TEXT;
    }, false, false);

    menu.addOption("Confirm", [&]()
    {
        std::cout << "Successfully update frame max count. (press any key to return)" << std::endl;
        menu.endReceiveInput();
    }, false, true);

    menu.show();
    menu.startReceiveInput();

#undef UPDATE_MENU_TOP_TEXT
#undef CREATE_MENU_TOP_TEXT
}

int main(int argc, char* argv)
{
    if (!loadBlockEntries())
        return -1;

    CommandLineMenu menu;
    menu.setMaxColumn(3);

    menu.addOption("Generate Block Image",     &generateBlockImageTriggered);
    menu.addOption("Generate Block Video",     &generateBlockVideoTriggered);
    menu.addOption("Generate Image Structure", &generateImageStructureTriggered);
    menu.addOption("Generate Video Structure", &generateVideoStructureTriggered);
    menu.addOption("Generate Image Function",  &generateImageFunctionTriggered);
    menu.addOption("Generate Video Function",  &generateVideoFunctionTriggered);
    menu.addOption("Filter Blocks",            &filterBlocksTriggered,        true, false);
    menu.addOption("Select Fallback Block",    &selectFallbackBlockTriggered, true, false);
    menu.addOption("Limit Frame Size",         &limitFrameSizeTriggered,      true, false);
    menu.addOption("Limit Frame Count",        &limitFrameCountTriggered,     true, false);
    menu.addOption("Exit", [&]() { menu.endReceiveInput(); }, false, false);

    menu.show();
    menu.startReceiveInput();

    return 0;
}
