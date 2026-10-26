#pragma once

#include <qstring.h>

#include <block.hpp>

/** 获取给定方块属性对应的文本（已翻译）。 */
QString getBlockAttributeText(BlockAttribute attribute);

/** 获取给定方块属性筛选模式对应的文本（已翻译）。 */
QString getBlockAttributeFilterModeText(BlockAttributeFilterMode filterMode);

/** 构造给定方块属性对应的文本（已翻译）。 */
QString makeBlockAttributesText(BlockAttributes attributes);
