#include "curve.h"
#include <algorithm>
#include <cmath>

namespace RetroNode {

RN_REGISTER_CLASS(Curve);

Curve::Curve() {
    // Default curve: constant 1.0 from position 0.0 to 1.0
    points.push_back({0.0f, 1.0f});
    points.push_back({1.0f, 1.0f});
    is_sorted = true;
}

void Curve::sort_points() {
    if (!is_sorted) {
        std::stable_sort(points.begin(), points.end());
        is_sorted = true;
    }
}

void Curve::add_point(float position, float value) {
    position = std::clamp(position, 0.0f, 1.0f);
    points.push_back({position, value});
    is_sorted = false;
}

void Curve::remove_point(size_t index) {
    if (index < points.size()) {
        points.erase(points.begin() + index);
    }
}

void Curve::clear() {
    points.clear();
    is_sorted = true;
}

void Curve::set_point_position(size_t index, float position) {
    if (index < points.size()) {
        points[index].position = std::clamp(position, 0.0f, 1.0f);
        is_sorted = false;
    }
}

float Curve::get_point_position(size_t index) const {
    if (index < points.size()) {
        return points[index].position;
    }
    return 0.0f;
}

void Curve::set_point_value(size_t index, float value) {
    if (index < points.size()) {
        points[index].value = value;
    }
}

float Curve::get_point_value(size_t index) const {
    if (index < points.size()) {
        return points[index].value;
    }
    return 1.0f;
}

float Curve::sample(float offset) {
    if (points.empty()) {
        return 1.0f;
    }

    sort_points();

    offset = std::clamp(offset, 0.0f, 1.0f);

    if (offset <= points.front().position) {
        return points.front().value;
    }
    if (offset >= points.back().position) {
        return points.back().value;
    }

    for (size_t i = 0; i < points.size() - 1; ++i) {
        if (offset >= points[i].position && offset <= points[i + 1].position) {
            float range = points[i + 1].position - points[i].position;
            float t = (range > 0.00001f) ? ((offset - points[i].position) / range) : 0.0f;
            return points[i].value + (points[i + 1].value - points[i].value) * t;
        }
    }

    return points.back().value;
}

} // namespace RetroNode
