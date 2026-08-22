#pragma once

/** 方块属性，可使用逻辑或运算符进行组合。 */
enum BlockAttribute : int
{
    BLOCK_ATTRI_IS_UNCOMPELETED    = 0x00000001,
    BLOCK_ATTRI_IS_TRANSPARENT     = 0x00000002,
    BLOCK_ATTRI_HAS_GRAVITY        = 0x00000004,
    BLOCK_ATTRI_CAN_BURNED         = 0x00000008,
    BLOCK_ATTRI_CAN_ENDERMAN_TAKED = 0x00000010
};

/** 不具有任何方块属性 */
constexpr int BLOCK_ATTRI_NONE     = 0x00000000;
/** 具有所有方块属性 */
constexpr int BLOCK_ATTRI_ALL      = 0xFFFFFFFF;

using BlockAttributes = int;
