#include "collision_shape_2d.h"
#include "../../core/object/class_db.h"

namespace RetroNode {

RN_REGISTER_CLASS(CollisionShape2D)

CollisionShape2D::CollisionShape2D() {
    name = "CollisionShape2D";
}

Rect2Fixed CollisionShape2D::get_global_box() const {
    Vector2Fixed gpos = get_global_position();
    if (shape_type == ShapeType::RECTANGLE) {
        return Rect2Fixed(gpos + rect_shape.position, rect_shape.size);
    } else {
        // Circle bounding box centered at global position with offset
        Vector2Fixed diameter(radius * Fixed16(2), radius * Fixed16(2));
        Vector2Fixed top_left = gpos - Vector2Fixed(radius, radius);
        return Rect2Fixed(top_left, diameter);
    }
}

} // namespace RetroNode
