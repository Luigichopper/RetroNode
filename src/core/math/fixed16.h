#ifndef RETRONODE_FIXED16_H
#define RETRONODE_FIXED16_H

#include <cstdint>
#include <cassert>

namespace RetroNode {

struct Fixed16 {
    int32_t raw;

    constexpr Fixed16() : raw(0) {}
    constexpr explicit Fixed16(int32_t r) : raw(r) {}

    // Float conversion (primarily for rendering / delta input)
    static constexpr Fixed16 from_float(float f) {
        return Fixed16(static_cast<int32_t>(f * 65536.0f));
    }

    float to_float() const {
        return static_cast<float>(raw) / 65536.0f;
    }

    // Integer conversion helpers
    static constexpr Fixed16 from_int(int32_t i) {
        return Fixed16(i << 16);
    }

    int32_t to_int() const {
        return raw >> 16;
    }

    // Operator overloads for determinism
    constexpr Fixed16 operator+(const Fixed16& o) const { return Fixed16(raw + o.raw); }
    constexpr Fixed16 operator-(const Fixed16& o) const { return Fixed16(raw - o.raw); }

    constexpr Fixed16 operator*(const Fixed16& o) const {
        return Fixed16(static_cast<int32_t>((static_cast<int64_t>(raw) * o.raw) >> 16));
    }

    constexpr Fixed16 operator/(const Fixed16& o) const {
        assert(o.raw != 0 && "Fixed16 division by zero");
        if (o.raw == 0) return Fixed16(0);
        return Fixed16(static_cast<int32_t>((static_cast<int64_t>(raw) << 16) / o.raw));
    }

    constexpr Fixed16& operator+=(const Fixed16& o) { raw += o.raw; return *this; }
    constexpr Fixed16& operator-=(const Fixed16& o) { raw -= o.raw; return *this; }

    constexpr Fixed16& operator*=(const Fixed16& o) {
        raw = static_cast<int32_t>((static_cast<int64_t>(raw) * o.raw) >> 16);
        return *this;
    }

    constexpr Fixed16& operator/=(const Fixed16& o) {
        assert(o.raw != 0 && "Fixed16 division by zero");
        if (o.raw != 0) {
            raw = static_cast<int32_t>((static_cast<int64_t>(raw) << 16) / o.raw);
        }
        return *this;
    }

    constexpr Fixed16 operator-() const { return Fixed16(-raw); }

    constexpr bool operator==(const Fixed16& o) const { return raw == o.raw; }
    constexpr bool operator!=(const Fixed16& o) const { return raw != o.raw; }
    constexpr bool operator<(const Fixed16& o) const { return raw < o.raw; }
    constexpr bool operator>(const Fixed16& o) const { return raw > o.raw; }
    constexpr bool operator<=(const Fixed16& o) const { return raw <= o.raw; }
    constexpr bool operator>=(const Fixed16& o) const { return raw >= o.raw; }
};

} // namespace RetroNode

#endif // RETRONODE_FIXED16_H
