#include <iostream>
#include <string>

#include <opencv2/imgcodecs.hpp>
#include <mcbe_toolbox_api.hpp>

#include "command_line_menu.hpp"

struct Configurations
{
    BlockEntryMap            blockEntries;
    BlockAttributes          filterAttributes =
        BLOCK_ATTRI_HAS_PATTERN   |
        BLOCK_ATTRI_IS_INCOMPLETE |
        BLOCK_ATTRI_IS_TRANSPARENT;
    BlockAttributeFilterMode filterMode = BLOCK_ATTRI_FILTER_MODE_DISJOINT;
    BlockDataMap             blocks;
    const BlockDataPair*     fallbackBlock = nullptr;
    cv::Size                 frameMaxSize  = cv::Size(1080, 1080);
    int                      frameMaxCount = -1;
};

static Configurations config;

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

void progressCllback(std::size_t current, std::size_t total, bool& stop, void* userdata)
{
    const std::size_t step = *static_cast<std::size_t*>(userdata);
    if (current % step == 0 || current == total)
        std::cout << "[" << current << "/" << total << "]\n";
}

SurfaceDirection getDesiredSurface()
{

}

void generateBlockImageTriggered()
{
    BlockImageFactory factory(config.blocks, getDesiredSurface());
    factory.setFallbackBlock(config.fallbackBlock);
    factory.setProgressCallback(progressCllback);
    std::size_t callbackStep = 1000;
    factory.setUserData(static_cast<void*>(&callbackStep));

    std::string filepath;
    std::cout << "Please input the image filepath: " << std::endl;
    std::cin >> filepath;
    ImageFramesOStream stream(filepath, config.frameMaxSize.width, config.frameMaxSize.height);

    std::cout << "Start generate block image" << std::endl;
    const cv::Mat image = factory.generateBlockImage(stream);
    std::cout << "Block image generate finished" << std::endl;

    if (cv::imwrite("./out.png", image))
        std::cout << "Successfully save the block image to './out.png'" << std::endl;
    else
        std::cerr << "Failed to save the block image to './out.png'" << std::endl;
}

void generateImageStructureTriggered()
{

}

void generateVideoStructureTriggered()
{

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
    }, false, false);

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
    }, false, false);

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
    }, false, false);

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
    }, false, false);

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
    menu.addOption("Generate Image Structure", &generateImageStructureTriggered);
    menu.addOption("Generate Video Structure", &generateVideoStructureTriggered);
    menu.addOption("Filter Blocks",            &filterBlocksTriggered);
    menu.addOption("Select Fallback Block",    &selectFallbackBlockTriggered);
    menu.addOption("Limit Frame Size",         &limitFrameSizeTriggered);
    menu.addOption("Limit Frame Count",        &limitFrameCountTriggered);
    menu.addOption("Exit", [&]() { menu.endReceiveInput(); }, false, false);

    menu.show();
    menu.startReceiveInput();
}
