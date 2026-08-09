#ifndef RETRONODE_FIXED16_H
#define RETRONODE_FIXED16_H

#include <cstdint>

namespace RetroNode {

struct Fixed16 {
    int32_t raw;

    constexpr Fixed16() noexcept : raw(0) {}
    constexpr explicit Fixed16(int32_t r) noexcept : raw(r) {}

    // Float conversion (primarily for rendering / delta input)
    static constexpr Fixed16 from_float(float f) noexcept {
        return Fixed16(static_cast<int32_t>(f * 65536.0f));
    }

    constexpr float to_float() const noexcept {
        return static_cast<float>(raw) / 65536.0f;
    }

    // Integer conversion helpers
    static constexpr Fixed16 from_int(int32_t i) noexcept {
        return Fixed16(i << 16);
    }

    constexpr int32_t to_int() const noexcept {
        return raw >> 16;
    }

    // Pass-by-value operator overloads with noexcept & rounding
    constexpr Fixed16 operator+(Fixed16 o) const noexcept { return Fixed16(raw + o.raw); }
    constexpr Fixed16 operator-(Fixed16 o) const noexcept { return Fixed16(raw - o.raw); }

    // Round-half-up multiplication: (a * b + 0x8000) >> 16
    constexpr Fixed16 operator*(Fixed16 o) const noexcept {
        int64_t prod = static_cast<int64_t>(raw) * o.raw;
        return Fixed16(static_cast<int32_t>((prod + 0x8000) >> 16));
    }

    // Safe constexpr division with rounding & zero protection
    constexpr Fixed16 operator/(Fixed16 o) const noexcept {
        if (o.raw == 0) return Fixed16(0);
        int64_t num = static_cast<int64_t>(raw) << 16;
        int64_t half_denom = o.raw / 2;
        int64_t res = (num >= 0) ? ((num + half_denom) / o.raw) : ((num - half_denom) / o.raw);
        return Fixed16(static_cast<int32_t>(res));
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

    constexpr Fixed16 operator-() const noexcept { return Fixed16(-raw); }

    constexpr bool operator==(Fixed16 o) const noexcept { return raw == o.raw; }
    constexpr bool operator!=(Fixed16 o) const noexcept { return raw != o.raw; }
    constexpr bool operator<(Fixed16 o) const noexcept { return raw < o.raw; }
    constexpr bool operator>(Fixed16 o) const noexcept { return raw > o.raw; }
    constexpr bool operator<=(Fixed16 o) const noexcept { return raw <= o.raw; }
    constexpr bool operator>=(Fixed16 o) const noexcept { return raw >= o.raw; }
};

} // namespace RetroNode

#endif // RETRONODE_FIXED16_H
