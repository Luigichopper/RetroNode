#ifndef RETRONODE_CURVE_H
#define RETRONODE_CURVE_H

#include <vector>
#include <algorithm>
#include "../object/class_db.h"

namespace RetroNode {

struct CurvePoint {
    float position = 0.0f; // 0.0 to 1.0
    float value = 1.0f;

    bool operator<(const CurvePoint& other) const {
        return position < other.position;
    }
};

class RN_API Curve : public Object {
    RN_CLASS(Curve, Object)

private:
    std::vector<CurvePoint> points;
    bool is_sorted = true;

    void sort_points();

public:
    Curve();
    virtual ~Curve() = default;

    void add_point(float position, float value);
    void remove_point(size_t index);
    void clear();

    size_t get_point_count() const { return points.size(); }
    const std::vector<CurvePoint>& get_points() const { return points; }

    void set_point_position(size_t index, float position);
    float get_point_position(size_t index) const;

    void set_point_value(size_t index, float value);
    float get_point_value(size_t index) const;

    float sample(float offset);
};

} // namespace RetroNode

#endif // RETRONODE_CURVE_H
