#include "block_attribute_text.h"

#include <easy_translate.hpp>

QString getBlockAttributeText(BlockAttribute attribute)
{
    switch (attribute)
    {
        case BLOCK_ATTRI_IS_INCOMPLETE:     return EASYTR("Incomplete Block");
        case BLOCK_ATTRI_IS_TRANSPARENT:    return EASYTR("Transparent Block");
        case BLOCK_ATTRI_IS_LUMINOUS:       return EASYTR("Luminous Block");
        case BLOCK_ATTRI_IS_UNSTABLE:       return EASYTR("Unstable Block");
        case BLOCK_ATTRI_IS_CREATIVE:       return EASYTR("Creative Block");
        case BLOCK_ATTRI_HAS_GRAVITY:       return EASYTR("Has Gravity");
        case BLOCK_ATTRI_HAS_PATTERN:       return EASYTR("Has Pattern");
        case BLOCK_ATTRI_FLAMMABLE:         return EASYTR("Flammable");
        case BLOCK_ATTRI_ENDERMAN_PICKABLE: return EASYTR("Enderman Pickable");
        default:                            return EASYTR("Unknown Block Attribute");
    }
}

QString getBlockAttributeFilterModeText(BlockAttributeFilterMode filterMode)
{
    switch (filterMode)
    {
        case BLOCK_ATTRI_FILTER_MODE_CONTAINS_ANY: return EASYTR("Contains Any Checked Attribute");
        case BLOCK_ATTRI_FILTER_MODE_CONTAINS_ALL: return EASYTR("Contains All Checked Attributes");
        default:                                   return EASYTR("Unknown Filter Mode");
    }
}

QString makeBlockAttributesText(BlockAttributes attributes)
{
#define APPEND_ATTRIBUTE_TEXT(attribute)                                        \
    if (HAS_BLOCK_ATTRIS(attributes, attribute)) text =                         \
        text +                                                                  \
        (text.isEmpty() ? QString() : QStringLiteral("; ")) +                   \
        getBlockAttributeText(attribute)

    QString text;
    APPEND_ATTRIBUTE_TEXT(BLOCK_ATTRI_IS_INCOMPLETE);
    APPEND_ATTRIBUTE_TEXT(BLOCK_ATTRI_IS_TRANSPARENT);
    APPEND_ATTRIBUTE_TEXT(BLOCK_ATTRI_IS_LUMINOUS);
    APPEND_ATTRIBUTE_TEXT(BLOCK_ATTRI_IS_UNSTABLE);
    APPEND_ATTRIBUTE_TEXT(BLOCK_ATTRI_IS_CREATIVE);
    APPEND_ATTRIBUTE_TEXT(BLOCK_ATTRI_HAS_GRAVITY);
    APPEND_ATTRIBUTE_TEXT(BLOCK_ATTRI_HAS_PATTERN);
    APPEND_ATTRIBUTE_TEXT(BLOCK_ATTRI_FLAMMABLE);
    APPEND_ATTRIBUTE_TEXT(BLOCK_ATTRI_ENDERMAN_PICKABLE);
    return text.isEmpty() ? EASYTR("None Block Attributes") : text;

#undef APPEND_ATTRIBUTE_TEXT
}
