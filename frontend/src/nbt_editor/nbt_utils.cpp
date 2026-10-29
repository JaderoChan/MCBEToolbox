#include "nbt_utils.h"

#include <qmap.h>

namespace nbt_utils
{

// 将字符串中的换行符转换为可视符号。
QString replaceNewLineChar(QString text)
{
    text.replace(QStringLiteral("\r\n"), QString(QChar(0x21B5)));
    text.replace(QChar(u'\r'), QChar(0x21B5));
    text.replace(QChar(u'\n'), QChar(0x21B5));
    return text;
}

const QList<nbt::TagType>& getAddableTagTypes(const nbt::Tag* tag)
{
    using RetType = QList<nbt::TagType>;

    static const RetType emptyTypes{};
    static const RetType byteTypes{nbt::TT_BYTE};
    static const RetType shortTypes{nbt::TT_SHORT};
    static const RetType intTypes{nbt::TT_INT};
    static const RetType longTypes{nbt::TT_LONG};
    static const RetType floatTypes{nbt::TT_FLOAT};
    static const RetType doubleTypes{nbt::TT_DOUBLE};
    static const RetType stringTypes{nbt::TT_STRING};
    static const RetType byteArrayTypes{nbt::TT_BYTE_ARRAY};
    static const RetType intArrayTypes{nbt::TT_INT_ARRAY};
    static const RetType longArrayTypes{nbt::TT_LONG_ARRAY};
    static const RetType listTypes{nbt::TT_LIST};
    static const RetType compoundTypes{nbt::TT_COMPOUND};
    static const QList<nbt::TagType> fullTypes
    {
        nbt::TT_BYTE, nbt::TT_SHORT, nbt::TT_INT, nbt::TT_LONG, nbt::TT_FLOAT, nbt::TT_DOUBLE,
        nbt::TT_STRING, nbt::TT_BYTE_ARRAY, nbt::TT_INT_ARRAY, nbt::TT_LONG_ARRAY,
        nbt::TT_LIST, nbt::TT_COMPOUND
    };

    static const QMap<nbt::TagType, const RetType*> typeMapping
    {
        {nbt::TT_BYTE,       &byteTypes},
        {nbt::TT_SHORT,      &shortTypes},
        {nbt::TT_INT,        &intTypes},
        {nbt::TT_LONG,       &longTypes},
        {nbt::TT_FLOAT,      &floatTypes},
        {nbt::TT_DOUBLE,     &doubleTypes},
        {nbt::TT_STRING,     &stringTypes},
        {nbt::TT_BYTE_ARRAY, &byteArrayTypes},
        {nbt::TT_INT_ARRAY,  &intArrayTypes},
        {nbt::TT_LONG_ARRAY, &longArrayTypes},
        {nbt::TT_LIST,       &listTypes},
        {nbt::TT_COMPOUND,   &compoundTypes},
    };

    if (!tag)
        return fullTypes;

    // 当 List 的子元素类型为 TT_END 时意味着此 List 可用任意类型进行初始化，否则只允许添加子元素类型。
    if (tag->type() == nbt::TT_LIST)
        return tag->listItemType() == nbt::TT_END ? fullTypes : *typeMapping[tag->listItemType()];

    switch (tag->type())
    {
        case nbt::TT_BYTE_ARRAY: return *typeMapping[nbt::TT_BYTE];
        case nbt::TT_INT_ARRAY:  return *typeMapping[nbt::TT_INT];
        case nbt::TT_LONG_ARRAY: return *typeMapping[nbt::TT_LONG];
        case nbt::TT_COMPOUND:   return fullTypes;
        default:                 return emptyTypes;
    }
}

const QIcon& getTagTypeIcon(nbt::TagType type)
{
    static const QIcon icons[] =
    {
        QIcon(),
        QIcon(QStringLiteral(":/icons/nbt_editor/byte.svg")),
        QIcon(QStringLiteral(":/icons/nbt_editor/short.svg")),
        QIcon(QStringLiteral(":/icons/nbt_editor/int.svg")),
        QIcon(QStringLiteral(":/icons/nbt_editor/long.svg")),
        QIcon(QStringLiteral(":/icons/nbt_editor/float.svg")),
        QIcon(QStringLiteral(":/icons/nbt_editor/double.svg")),
        QIcon(QStringLiteral(":/icons/nbt_editor/byte_array.svg")),
        QIcon(QStringLiteral(":/icons/nbt_editor/string.svg")),
        QIcon(QStringLiteral(":/icons/nbt_editor/list.svg")),
        QIcon(QStringLiteral(":/icons/nbt_editor/compound.svg")),
        QIcon(QStringLiteral(":/icons/nbt_editor/int_array.svg")),
        QIcon(QStringLiteral(":/icons/nbt_editor/long_array.svg")),
    };
    return icons[static_cast<int>(type)];
}

QString makeTextFromTag(const nbt::Tag& tag)
{
    switch (tag.type())
    {
        case nbt::TT_BYTE:   return QString::number(tag.getByte());
        case nbt::TT_SHORT:  return QString::number(tag.getShort());
        case nbt::TT_INT:    return QString::number(tag.getInt());
        case nbt::TT_LONG:   return QString::number(tag.getLong());
        case nbt::TT_FLOAT:  return QString::number(tag.getFloat());
        case nbt::TT_DOUBLE: return QString::number(tag.getDouble());
        case nbt::TT_STRING: return replaceNewLineChar(QString::fromStdString(tag.getString()));
        default:             return QString();
    }
}

nbt::Tag parseTextToTag(nbt::TagType type, const QString& text, bool& ok)
{
    const auto& trimmed = text.trimmed();
    switch (type)
    {
        case nbt::TT_BYTE:   return nbt::Tag(static_cast<int8_t>(trimmed.toLongLong(&ok)));
        case nbt::TT_SHORT:  return nbt::Tag(static_cast<int16_t>(trimmed.toLongLong(&ok)));
        case nbt::TT_INT:    return nbt::Tag(static_cast<int32_t>(trimmed.toLongLong(&ok)));
        case nbt::TT_LONG:   return nbt::Tag(static_cast<int64_t>(trimmed.toLongLong(&ok)));
        case nbt::TT_FLOAT:  return nbt::Tag(static_cast<float>(trimmed.toDouble(&ok)));
        case nbt::TT_DOUBLE: return nbt::Tag(trimmed.toDouble(&ok));
        case nbt::TT_STRING: ok = true;  return nbt::Tag(text.toStdString());
        default:             ok = false; return nbt::Tag(type);
    }
}

QString makeArrayElementText(const nbt::Tag& arrayTag, qulonglong index)
{
    switch (arrayTag.type())
    {
        case nbt::TT_BYTE_ARRAY: return QString::number(arrayTag.getByteArray()[index]);
        case nbt::TT_INT_ARRAY:  return QString::number(arrayTag.getIntArray()[index]);
        case nbt::TT_LONG_ARRAY: return QString::number(arrayTag.getLongArray()[index]);
        default:                 return QString();
    }
}

nbt::TagType getArrayElementTagType(nbt::TagType arrayType)
{
    switch (arrayType)
    {
        case nbt::TT_BYTE_ARRAY: return nbt::TT_BYTE;
        case nbt::TT_INT_ARRAY:  return nbt::TT_INT;
        case nbt::TT_LONG_ARRAY: return nbt::TT_LONG;
        default:                 return nbt::TT_END;
    }
}

} // namespace nbt_utils
