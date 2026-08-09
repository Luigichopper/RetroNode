#ifndef RETRONODE_COLLISION_SHAPE_2D_H
#define RETRONODE_COLLISION_SHAPE_2D_H

#include "../2d/node_2d.h"
#include "../../core/math/rect2.h"
#include "../../core/math/fixed16.h"

namespace RetroNode {

enum class ShapeType {
    RECTANGLE,
    CIRCLE
};

class RN_API CollisionShape2D : public Node2D {
    RN_CLASS(CollisionShape2D, Node2D)

private:
    ShapeType shape_type = ShapeType::RECTANGLE;
    Rect2Fixed rect_shape = Rect2Fixed(Vector2Fixed::zero(), Vector2Fixed::from_floats(16.0f, 16.0f));
    Fixed16 radius = Fixed16::from_float(8.0f);

public:
    CollisionShape2D();
    ~CollisionShape2D() override = default;

    void set_shape_type(ShapeType p_type) { shape_type = p_type; }
    ShapeType get_shape_type() const { return shape_type; }

    void set_rect(const Rect2Fixed& p_rect) { rect_shape = p_rect; }
    Rect2Fixed get_rect() const { return rect_shape; }

    void set_radius(Fixed16 p_radius) { radius = p_radius; }
    Fixed16 get_radius() const { return radius; }

    Rect2Fixed get_global_box() const;
};

} // namespace RetroNode

#endif // RETRONODE_COLLISION_SHAPE_2D_H
