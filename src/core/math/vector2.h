#ifndef RETRONODE_VECTOR2_H
#define RETRONODE_VECTOR2_H

#include "fixed16.h"

namespace RetroNode {

struct Vector2Fixed {
    Fixed16 x;
    Fixed16 y;

    constexpr Vector2Fixed() noexcept : x(0), y(0) {}
    constexpr Vector2Fixed(Fixed16 _x, Fixed16 _y) noexcept : x(_x), y(_y) {}

    static constexpr Vector2Fixed zero() noexcept {
        return Vector2Fixed(Fixed16(0), Fixed16(0));
    }

    static constexpr Vector2Fixed one() noexcept {
        return Vector2Fixed(Fixed16::from_int(1), Fixed16::from_int(1));
    }

    static constexpr Vector2Fixed from_floats(float fx, float fy) noexcept {
        return Vector2Fixed(Fixed16::from_float(fx), Fixed16::from_float(fy));
    }

    constexpr Vector2Fixed operator+(Vector2Fixed o) const noexcept {
        return Vector2Fixed(x + o.x, y + o.y);
    }

    constexpr Vector2Fixed operator-(Vector2Fixed o) const noexcept {
        return Vector2Fixed(x - o.x, y - o.y);
    }

    constexpr Vector2Fixed operator*(Fixed16 scalar) const noexcept {
        return Vector2Fixed(x * scalar, y * scalar);
    }

    constexpr Vector2Fixed operator/(Fixed16 scalar) const noexcept {
        return Vector2Fixed(x / scalar, y / scalar);
    }

    constexpr Vector2Fixed& operator+=(Vector2Fixed o) noexcept {
        x += o.x;
        y += o.y;
        return *this;
    }

    constexpr Vector2Fixed& operator-=(Vector2Fixed o) noexcept {
        x -= o.x;
        y -= o.y;
        return *this;
    }

    constexpr Vector2Fixed& operator*=(Fixed16 scalar) noexcept {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    constexpr Vector2Fixed& operator/=(Fixed16 scalar) noexcept {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    constexpr Vector2Fixed operator-() const noexcept {
        return Vector2Fixed(-x, -y);
    }

    constexpr bool operator==(Vector2Fixed o) const noexcept {
        return x == o.x && y == o.y;
    }

    constexpr bool operator!=(Vector2Fixed o) const noexcept {
        return !(*this == o);
    }
};

} // namespace RetroNode

#endif // RETRONODE_VECTOR2_H
