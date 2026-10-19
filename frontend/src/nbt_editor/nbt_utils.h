#pragma once

#include <qicon.h>
#include <qlist.h>
#include <qstring.h>

#include <mcnbt/mcnbt.hpp>

namespace nbt_utils
{

// 判断给定类型是否为标量。
static inline bool isScalarTagType(nbt::TagType type)
{
    return
        type == nbt::TT_BYTE || type == nbt::TT_SHORT || type == nbt::TT_INT    ||
        type == nbt::TT_LONG || type == nbt::TT_FLOAT || type == nbt::TT_DOUBLE ||
        type == nbt::TT_STRING;
}

// 判断给定类型的键名是否可编辑。
static inline bool tagKeyEditable(nbt::TagType type, bool isNamingTag)
{
    return isNamingTag && type != nbt::TT_END;
}

// 判断给定类型的键值是否可编辑。
static inline bool tagValueEditable(nbt::TagType type, bool isNamingTag)
{
    (void)isNamingTag;
    return isScalarTagType(type);
}

// 判断给定类型是否可编辑（键名/键值任一可编辑）。
static inline bool tagEditable(nbt::TagType type, bool isNamingTag)
{
    return tagKeyEditable(type, isNamingTag) || tagValueEditable(type, isNamingTag);
}

// 获取指定 Tag 可添加的子元素类型列表；如果参数 tag 为 nullptr 则返回除 TT_END 外所有类型组成的列表。
const QList<nbt::TagType>& addableTagTypes(const nbt::Tag* tag = nullptr);

// 获取指定类型的图标；如果参数 type 为 TT_END 返回空 QIcon。
const QIcon& tagTypeIcon(nbt::TagType type);

// 获取指定标量 Tag 的文本；如果参数 tag 不是标量返回空字符串。
QString tagText(const nbt::Tag& tag);

// 解析指定文本至标量 Tag；如果文本无法解析为 Tag，参数 ok 将置 false。
nbt::Tag parseTextToTag(nbt::TagType type, const QString& text, bool& ok);

// 获取指定数组 Tag 给定位置子元素的文本；如果参数 tag 非数组类型返回空字符串。
QString arrayElementText(const nbt::Tag& arrayTag, qulonglong index);

// 获取指定数组类型的子元素类型；如果参数 tag 非数组类型返回 TT_END。
nbt::TagType arrayElementTagType(nbt::TagType arrayType);

} // namespace nbt_utils
