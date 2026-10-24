#include "block_attributes_text.h"

#include <easy_translate.hpp>

namespace
{

inline const char* getBlockAttributeRawText(BlockAttribute attribute)
{
    switch (attribute)
    {
        case BLOCK_ATTRI_IS_INCOMPLETE:     return "Incomplete Block";
        case BLOCK_ATTRI_IS_TRANSPARENT:    return "Transparent Block";
        case BLOCK_ATTRI_IS_LUMINOUS:       return "Luminous Block";
        case BLOCK_ATTRI_IS_UNSTABLE:       return "Unstable Block";
        case BLOCK_ATTRI_IS_CREATIVE:       return "Creative Block";
        case BLOCK_ATTRI_HAS_GRAVITY:       return "Has Gravity";
        case BLOCK_ATTRI_HAS_PATTERN:       return "Has Pattern";
        case BLOCK_ATTRI_FLAMMABLE:         return "Flammable";
        case BLOCK_ATTRI_ENDERMAN_PICKABLE: return "Enderman Pickable";
        default:                            return "Unknown Block Attribute";
    }
}

} // namespace

QString makeBlockAttributesText(BlockAttributes attributes)
{
#define APPEND_ATTRIBUTE_TEXT(attribute)                                        \
    if (HAS_BLOCK_ATTRIS(attributes, attribute)) text =                         \
        text +                                                                  \
        (text.isEmpty() ? "" : "; ") +                                          \
        EASYTR(getBlockAttributeRawText(attribute))

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
