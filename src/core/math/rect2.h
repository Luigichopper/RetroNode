#ifndef RETRONODE_RECT2_H
#define RETRONODE_RECT2_H

#include "vector2.h"

namespace RetroNode {

struct Rect2Fixed {
    Vector2Fixed position;
    Vector2Fixed size;

    constexpr Rect2Fixed() : position(), size() {}
    constexpr Rect2Fixed(Vector2Fixed pos, Vector2Fixed sz) : position(pos), size(sz) {}
    constexpr Rect2Fixed(Fixed16 x, Fixed16 y, Fixed16 w, Fixed16 h)
        : position(x, y), size(w, h) {}

    static constexpr Rect2Fixed from_floats(float x, float y, float w, float h) {
        return Rect2Fixed(
            Fixed16::from_float(x),
            Fixed16::from_float(y),
            Fixed16::from_float(w),
            Fixed16::from_float(h)
        );
    }

    constexpr Vector2Fixed end() const {
        return position + size;
    }

    constexpr bool has_point(const Vector2Fixed& point) const {
        if (point.x < position.x || point.y < position.y) return false;
        if (point.x >= position.x + size.x || point.y >= position.y + size.y) return false;
        return true;
    }

    constexpr bool intersects(const Rect2Fixed& o) const {
        if (position.x >= o.position.x + o.size.x || o.position.x >= position.x + size.x) return false;
        if (position.y >= o.position.y + o.size.y || o.position.y >= position.y + size.y) return false;
        return true;
    }
};

} // namespace RetroNode

#endif // RETRONODE_RECT2_H
