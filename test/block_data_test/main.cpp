#include <stdio.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <block_data.hpp>

int main(int argc, char *argv[])
{
    if (argc == 2)
        std::filesystem::current_path(argv[1]);

    std::vector<std::pair<std::string, bool>> meta;
    {
        if (!std::filesystem::exists("./meta.json"))
        {
            printf("Current path not exists 'meta.json' file.\n");
            return 1;
        }
        std::ifstream metaFile("./meta.json");
        if (!metaFile.is_open())
        {
            printf("Failed to open the 'meta.json' file.\n");
            return 1;
        }

        std::string json(
            (std::istreambuf_iterator<char>(metaFile)),
            std::istreambuf_iterator<char>());
        metaFile.close();
        nlohmann::json j = nlohmann::json::parse(json);
        if (j.is_discarded() || !j.is_object())
        {
            printf("Invalid 'meta.json' file");
            return 1;
        }

        for (const auto& [path, result] : j.items())
        {
            if (result.is_boolean())
                meta.push_back({path, result});
        }
    }

    for (const auto& pair : meta)
    {
        if (std::filesystem::exists(pair.first))
        {
            std::ifstream testFile(pair.first);
            if (!testFile.is_open())
            {
                printf("- Failed open the file: %s\n", pair.first.c_str());
                continue;
            }

            std::string json(
                (std::istreambuf_iterator<char>(testFile)),
                std::istreambuf_iterator<char>());
            testFile.close();
            printf("- '%s': ", pair.first.c_str());
            try
            {
                BlockDatas bds = parseBlockDatasJson(json);
                printf(
                    "parse Success, expect result: %s\n",
                    (pair.second ? "Success" : "Fail"));
            }
            catch (std::exception& e)
            {
                printf(
                    "parse Fail (%s), expect result: %s\n",
                    e.what(),
                    (pair.second ? "Success" : "Fail"));
            }
        }
    }

    return 0;
}
