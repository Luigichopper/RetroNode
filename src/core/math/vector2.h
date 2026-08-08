#ifndef RETRONODE_VECTOR2_H
#define RETRONODE_VECTOR2_H

#include "fixed16.h"

namespace RetroNode {

struct Vector2Fixed {
    Fixed16 x;
    Fixed16 y;

    constexpr Vector2Fixed() : x(0), y(0) {}
    constexpr Vector2Fixed(Fixed16 _x, Fixed16 _y) : x(_x), y(_y) {}

    static constexpr Vector2Fixed zero() {
        return Vector2Fixed(Fixed16(0), Fixed16(0));
    }

    static constexpr Vector2Fixed from_floats(float fx, float fy) {
        return Vector2Fixed(Fixed16::from_float(fx), Fixed16::from_float(fy));
    }

    constexpr Vector2Fixed operator+(const Vector2Fixed& o) const {
        return Vector2Fixed(x + o.x, y + o.y);
    }

    constexpr Vector2Fixed operator-(const Vector2Fixed& o) const {
        return Vector2Fixed(x - o.x, y - o.y);
    }

    constexpr Vector2Fixed operator*(const Fixed16& scalar) const {
        return Vector2Fixed(x * scalar, y * scalar);
    }

    constexpr Vector2Fixed operator/(const Fixed16& scalar) const {
        return Vector2Fixed(x / scalar, y / scalar);
    }

    constexpr Vector2Fixed& operator+=(const Vector2Fixed& o) {
        x += o.x;
        y += o.y;
        return *this;
    }

    constexpr Vector2Fixed& operator-=(const Vector2Fixed& o) {
        x -= o.x;
        y -= o.y;
        return *this;
    }

    constexpr Vector2Fixed& operator*=(const Fixed16& scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    constexpr Vector2Fixed& operator/=(const Fixed16& scalar) {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    constexpr Vector2Fixed operator-() const {
        return Vector2Fixed(-x, -y);
    }

    constexpr bool operator==(const Vector2Fixed& o) const {
        return x == o.x && y == o.y;
    }

    constexpr bool operator!=(const Vector2Fixed& o) const {
        return x != o.x || y != o.y;
    }

    constexpr Fixed16 dot(const Vector2Fixed& o) const {
        return (x * o.x) + (y * o.y);
    }
};

} // namespace RetroNode

#endif // RETRONODE_VECTOR2_H
