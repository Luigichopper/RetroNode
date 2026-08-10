#include "gradient.h"
#include <algorithm>
#include <cmath>

namespace RetroNode {

RN_REGISTER_CLASS(Gradient);

Gradient::Gradient() {
    // Default gradient: White to Black or White to Transparent White
    points.push_back({0.0f, {255, 255, 255, 255}});
    points.push_back({1.0f, {255, 255, 255, 0}});
    is_sorted = true;
}

void Gradient::sort_points() {
    if (!is_sorted) {
        std::stable_sort(points.begin(), points.end());
        is_sorted = true;
    }
}

void Gradient::add_point(float offset, const SDL_Color& color) {
    offset = std::clamp(offset, 0.0f, 1.0f);
    points.push_back({offset, color});
    is_sorted = false;
}

void Gradient::remove_point(size_t index) {
    if (index < points.size()) {
        points.erase(points.begin() + index);
    }
}

void Gradient::clear() {
    points.clear();
    is_sorted = true;
}

void Gradient::set_point_offset(size_t index, float offset) {
    if (index < points.size()) {
        points[index].offset = std::clamp(offset, 0.0f, 1.0f);
        is_sorted = false;
    }
}

float Gradient::get_point_offset(size_t index) const {
    if (index < points.size()) {
        return points[index].offset;
    }
    return 0.0f;
}

void Gradient::set_point_color(size_t index, const SDL_Color& color) {
    if (index < points.size()) {
        points[index].color = color;
    }
}

SDL_Color Gradient::get_point_color(size_t index) const {
    if (index < points.size()) {
        return points[index].color;
    }
    return {255, 255, 255, 255};
}

SDL_Color Gradient::sample(float offset) {
    if (points.empty()) {
        return {255, 255, 255, 255};
    }

    sort_points();

    offset = std::clamp(offset, 0.0f, 1.0f);

    if (offset <= points.front().offset) {
        return points.front().color;
    }
    if (offset >= points.back().offset) {
        return points.back().color;
    }

    // Find interpolation segment
    for (size_t i = 0; i < points.size() - 1; ++i) {
        if (offset >= points[i].offset && offset <= points[i + 1].offset) {
            float range = points[i + 1].offset - points[i].offset;
            float t = (range > 0.00001f) ? ((offset - points[i].offset) / range) : 0.0f;

            const SDL_Color& c1 = points[i].color;
            const SDL_Color& c2 = points[i + 1].color;

            SDL_Color result;
            result.r = static_cast<Uint8>(c1.r + (c2.r - c1.r) * t);
            result.g = static_cast<Uint8>(c1.g + (c2.g - c1.g) * t);
            result.b = static_cast<Uint8>(c1.b + (c2.b - c1.b) * t);
            result.a = static_cast<Uint8>(c1.a + (c2.a - c1.a) * t);
            return result;
        }
    }

    return points.back().color;
}

} // namespace RetroNode
