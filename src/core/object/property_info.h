#ifndef RETRONODE_PROPERTY_INFO_H
#define RETRONODE_PROPERTY_INFO_H

#include "../string_names.h"
#include <string>

namespace RetroNode {

enum class VariantType {
    NIL,
    BOOL,
    INT,
    FLOAT16,      // Fixed16 native engine numeric type
    VECTOR2,
    RECT2,
    COLOR,
    STRING,
    STRING_NAME,
    OBJECT_REF
};

enum class PropertyHint {
    NONE,
    RANGE,
    FILE_PATH,
    MULTILINE_TEXT,
    ENUM,
    COLOR_NO_ALPHA
};

struct PropertyInfo {
    StringName name;
    VariantType type = VariantType::NIL;
    PropertyHint hint = PropertyHint::NONE;
    std::string hint_string;
};

} // namespace RetroNode

#endif // RETRONODE_PROPERTY_INFO_H
