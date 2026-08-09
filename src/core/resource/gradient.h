#ifndef RETRONODE_GRADIENT_H
#define RETRONODE_GRADIENT_H

#include <SDL3/SDL.h>
#include <vector>
#include <algorithm>
#include "../object/class_db.h"

namespace RetroNode {

struct GradientPoint {
    float offset = 0.0f; // 0.0 to 1.0
    SDL_Color color = {255, 255, 255, 255};

    bool operator<(const GradientPoint& other) const {
        return offset < other.offset;
    }
};

class RN_API Gradient : public Object {
    RN_CLASS(Gradient, Object)

private:
    std::vector<GradientPoint> points;
    bool is_sorted = true;

    void sort_points();

public:
    Gradient();
    virtual ~Gradient() = default;

    void add_point(float offset, const SDL_Color& color);
    void remove_point(size_t index);
    void clear();

    size_t get_point_count() const { return points.size(); }
    const std::vector<GradientPoint>& get_points() const { return points; }

    void set_point_offset(size_t index, float offset);
    float get_point_offset(size_t index) const;

    void set_point_color(size_t index, const SDL_Color& color);
    SDL_Color get_point_color(size_t index) const;

    SDL_Color sample(float offset);
};

} // namespace RetroNode

#endif // RETRONODE_GRADIENT_H
