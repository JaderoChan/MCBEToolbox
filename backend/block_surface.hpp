#pragma once

#include <string>

#include "color.hpp"

/** 描述方块各个面的材质文件路径与颜色 */
struct BlockSurface
{
    struct
    {
        std::string up, down, north, south, east, west;
    } textures;

    struct
    {
        Rgb up, down, north, south, east, west;
    } colors;
};
