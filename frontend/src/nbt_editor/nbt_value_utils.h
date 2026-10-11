#pragma once

#include <qlist.h>
#include <qvalidator.h>

#include <mcnbt/mcnbt.hpp>

// 数组标签（TT_BYTE_ARRAY/TT_INT_ARRAY/TT_LONG_ARRAY）的元素类型，非数组类型返回 TT_END。
nbt::TagType nbtArrayElementType(nbt::TagType type);

// 右键菜单中新增条目/元素时可选择的标签类型列表。
const QList<nbt::TagType>& nbtAddableTagTypes();

// 根据标签类型构造对应的数值输入校验器，不需要校验的类型（如字符串）返回 nullptr。
QValidator* nbtValidatorForType(nbt::TagType type, QObject* parent);
