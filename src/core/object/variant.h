#ifndef RETRONODE_OBJECT_VARIANT_H
#define RETRONODE_OBJECT_VARIANT_H

#include "property_info.h"
#include "../math/fixed16.h"
#include "../math/vector2.h"
#include "../math/rect2.h"
#include <string>
#include <SDL3/SDL.h>

namespace RetroNode {

class RN_API Variant {
private:
    VariantType type = VariantType::NIL;

    union {
        bool b;
        int64_t i;
        Fixed16 f16;
        Vector2Fixed v2;
        Rect2Fixed r2;
        SDL_Color color;
        uint64_t object_instance_id;
    };

    std::string str;

public:
    Variant() noexcept : type(VariantType::NIL), i(0) {}
    Variant(bool p_b) noexcept : type(VariantType::BOOL), b(p_b) {}
    Variant(int p_i) noexcept : type(VariantType::INT), i(p_i) {}
    Variant(int64_t p_i) noexcept : type(VariantType::INT), i(p_i) {}
    Variant(double p_f) noexcept : type(VariantType::FLOAT16), f16(Fixed16::from_float(static_cast<float>(p_f))) {}
    Variant(Fixed16 p_f16) noexcept : type(VariantType::FLOAT16), f16(p_f16) {}
    Variant(Vector2Fixed p_v2) noexcept : type(VariantType::VECTOR2), v2(p_v2) {}
    Variant(Rect2Fixed p_r2) noexcept : type(VariantType::RECT2), r2(p_r2) {}
    Variant(SDL_Color p_color) noexcept : type(VariantType::COLOR), color(p_color) {}
    Variant(const char* p_str) : type(VariantType::STRING), i(0), str(p_str ? p_str : "") {}
    Variant(const std::string& p_str) : type(VariantType::STRING), i(0), str(p_str) {}
    Variant(std::string&& p_str) noexcept : type(VariantType::STRING), i(0), str(std::move(p_str)) {}
    Variant(const StringName& p_sn) : type(VariantType::STRING_NAME), i(0), str(p_sn.as_string()) {}

    VariantType get_type() const noexcept { return type; }
    bool is_nil() const noexcept { return type == VariantType::NIL; }

    bool as_bool() const noexcept {
        if (type == VariantType::BOOL) return b;
        if (type == VariantType::INT) return i != 0;
        return false;
    }

    int64_t as_int() const noexcept {
        if (type == VariantType::INT) return i;
        if (type == VariantType::FLOAT16) return f16.to_int();
        if (type == VariantType::BOOL) return b ? 1 : 0;
        return 0;
    }

    Fixed16 as_fixed16() const noexcept {
        if (type == VariantType::FLOAT16) return f16;
        if (type == VariantType::INT) return Fixed16::from_int(static_cast<int32_t>(i));
        return Fixed16(0);
    }

    Vector2Fixed as_vector2() const noexcept {
        if (type == VariantType::VECTOR2) return v2;
        return Vector2Fixed::zero();
    }

    Rect2Fixed as_rect2() const noexcept {
        if (type == VariantType::RECT2) return r2;
        return Rect2Fixed();
    }

    SDL_Color as_color() const noexcept {
        if (type == VariantType::COLOR) return color;
        return { 255, 255, 255, 255 };
    }

    std::string as_string() const {
        if (type == VariantType::STRING || type == VariantType::STRING_NAME) return str;
        if (type == VariantType::INT) return std::to_string(i);
        if (type == VariantType::FLOAT16) return std::to_string(f16.to_float());
        if (type == VariantType::BOOL) return b ? "true" : "false";
        return "";
    }

    StringName as_string_name() const {
        return StringName(as_string());
    }

    // Backward compatibility helpers
    int to_int() const noexcept { return static_cast<int>(as_int()); }
    Fixed16 to_fixed16() const noexcept { return as_fixed16(); }
    float to_float() const noexcept { return as_fixed16().to_float(); }
    bool to_bool() const noexcept { return as_bool(); }
    std::string to_string() const { return as_string(); }
    Vector2Fixed to_vector2() const noexcept { return as_vector2(); }
    Rect2Fixed to_rect2() const noexcept { return as_rect2(); }
    SDL_Color to_color() const noexcept { return as_color(); }
};

} // namespace RetroNode

#endif // RETRONODE_OBJECT_VARIANT_H
