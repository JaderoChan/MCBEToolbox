#include "nbt_value_utils.h"

#include <limits>

#include <qregularexpression.h>

namespace
{

class IntegerValidator : public QValidator
{
public:
    IntegerValidator(qlonglong min, qlonglong max, QObject* parent)
        : QValidator(parent), min_(min), max_(max)
    {}

    State validate(QString& input, int&) const override
    {
        const QString trimmed = input.trimmed();
        if (trimmed.isEmpty() || trimmed == QStringLiteral("-"))
            return Intermediate;

        static const QRegularExpression digitsOnly(QStringLiteral("^-?\\d+$"));
        if (!digitsOnly.match(trimmed).hasMatch())
            return Invalid;

        bool ok = false;
        const qlonglong value = trimmed.toLongLong(&ok);
        if (!ok || value < min_ || value > max_)
            return Invalid;
        return Acceptable;
    }

private:
    qlonglong min_ = 0;
    qlonglong max_ = 0;
};

} // namespace

nbt::TagType nbtArrayElementType(nbt::TagType type)
{
    switch (type)
    {
        case nbt::TT_BYTE_ARRAY: return nbt::TT_BYTE;
        case nbt::TT_INT_ARRAY:  return nbt::TT_INT;
        case nbt::TT_LONG_ARRAY: return nbt::TT_LONG;
        default:                 return nbt::TT_END;
    }
}

const QList<nbt::TagType>& nbtAddableTagTypes()
{
    static const QList<nbt::TagType> types =
    {
        nbt::TT_BYTE, nbt::TT_SHORT, nbt::TT_INT, nbt::TT_LONG,
        nbt::TT_FLOAT, nbt::TT_DOUBLE, nbt::TT_STRING,
        nbt::TT_BYTE_ARRAY, nbt::TT_INT_ARRAY, nbt::TT_LONG_ARRAY,
        nbt::TT_LIST, nbt::TT_COMPOUND
    };
    return types;
}

QValidator* nbtValidatorForType(nbt::TagType type, QObject* parent)
{
    switch (type)
    {
        case nbt::TT_BYTE:
            return new IntegerValidator(
                std::numeric_limits<int8_t>::min(),
                std::numeric_limits<int8_t>::max(),
                parent
            );
        case nbt::TT_SHORT:
            return new IntegerValidator(
                std::numeric_limits<int16_t>::min(),
                std::numeric_limits<int16_t>::max(),
                parent
            );
        case nbt::TT_INT:
            return new IntegerValidator(
                std::numeric_limits<int32_t>::min(),
                std::numeric_limits<int32_t>::max(),
                parent
            );
        case nbt::TT_LONG:
            return new IntegerValidator(
                std::numeric_limits<int64_t>::min(),
                std::numeric_limits<int64_t>::max(),
                parent
            );
        case nbt::TT_FLOAT: // Fallthrough
        case nbt::TT_DOUBLE:
            return new QDoubleValidator(parent);
        default:
            return nullptr;
    }
}
