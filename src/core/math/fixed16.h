#ifndef RETRONODE_FIXED16_H
#define RETRONODE_FIXED16_H

#include <cstdint>

namespace RetroNode {

struct Fixed16 {
    int32_t raw;

    constexpr Fixed16() noexcept : raw(0) {}

    // Factory function for raw Q16.16 representation
    static constexpr Fixed16 from_raw(int32_t r) noexcept {
        Fixed16 f;
        f.raw = r;
        return f;
    }

    // Logical integer constructor: Fixed16(10) creates logical 10 (10 << 16)
    constexpr explicit Fixed16(int32_t val) noexcept : raw(val << 16) {}

    // Float conversion (primarily for rendering / delta input)
    static constexpr Fixed16 from_float(float f) noexcept {
        return from_raw(static_cast<int32_t>(f * 65536.0f));
    }

    constexpr float to_float() const noexcept {
        return static_cast<float>(raw) / 65536.0f;
    }

    // Integer conversion helpers
    static constexpr Fixed16 from_int(int32_t i) noexcept {
        return Fixed16(i);
    }

    constexpr int32_t to_int() const noexcept {
        return raw >> 16;
    }

    // Pass-by-value operator overloads with noexcept & rounding
    constexpr Fixed16 operator+(Fixed16 o) const noexcept { return from_raw(raw + o.raw); }
    constexpr Fixed16 operator-(Fixed16 o) const noexcept { return from_raw(raw - o.raw); }

    // Round-half-up multiplication: (a * b + 0x8000) >> 16
    constexpr Fixed16 operator*(Fixed16 o) const noexcept {
        int64_t prod = static_cast<int64_t>(raw) * o.raw;
        return from_raw(static_cast<int32_t>((prod + 0x8000) >> 16));
    }

    // Safe constexpr division with rounding & zero protection
    constexpr Fixed16 operator/(Fixed16 o) const noexcept {
        if (o.raw == 0) return Fixed16(0);
        int64_t num = static_cast<int64_t>(raw) << 16;
        int64_t half_denom = o.raw / 2;
        int64_t res = (num >= 0) ? ((num + half_denom) / o.raw) : ((num - half_denom) / o.raw);
        return from_raw(static_cast<int32_t>(res));
    }

    constexpr Fixed16& operator+=(Fixed16 o) noexcept { raw += o.raw; return *this; }
    constexpr Fixed16& operator-=(Fixed16 o) noexcept { raw -= o.raw; return *this; }

    constexpr Fixed16& operator*=(Fixed16 o) noexcept {
        *this = *this * o;
        return *this;
    }

    constexpr Fixed16& operator/=(Fixed16 o) noexcept {
        *this = *this / o;
        return *this;
    }

    constexpr Fixed16 operator-() const noexcept { return from_raw(-raw); }

    constexpr bool operator==(Fixed16 o) const noexcept { return raw == o.raw; }
    constexpr bool operator!=(Fixed16 o) const noexcept { return raw != o.raw; }
    constexpr bool operator<(Fixed16 o) const noexcept { return raw < o.raw; }
    constexpr bool operator>(Fixed16 o) const noexcept { return raw > o.raw; }
    constexpr bool operator<=(Fixed16 o) const noexcept { return raw <= o.raw; }
    constexpr bool operator>=(Fixed16 o) const noexcept { return raw >= o.raw; }
};

} // namespace RetroNode

#endif // RETRONODE_FIXED16_H
