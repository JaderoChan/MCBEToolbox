#pragma once

#include <string>

/** 描述方块各个面的材质文件路径与颜色 */
struct BlockSurface
{
    struct RGB
    {
        unsigned char r, g, b;
    };

    struct
    {
        std::string up, down, north, south, east, west;
    } textures;

    struct
    {
        RGB up, down, north, south, east, west;
    } colors;
};
