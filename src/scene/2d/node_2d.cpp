#include "node_2d.h"

namespace RetroNode {

Node2D::Node2D() 
    : position(Vector2Fixed::zero()), 
      previous_position(Vector2Fixed::zero()),
      rotation(0), 
      scale(Vector2Fixed::from_floats(1.0f, 1.0f)) {
    name = "Node2D";
}

Vector2Fixed Node2D::get_global_position() const {
    Vector2Fixed global_pos = position;
    const Node* p = get_parent();
    while (p) {
        const Node2D* n2d = dynamic_cast<const Node2D*>(p);
        if (n2d) {
            global_pos += n2d->position;
        }
        p = p->get_parent();
    }
    return global_pos;
}

Vector2Fixed Node2D::get_global_previous_position() const {
    Vector2Fixed global_prev = previous_position;
    const Node* p = get_parent();
    while (p) {
        const Node2D* n2d = dynamic_cast<const Node2D*>(p);
        if (n2d) {
            global_prev += n2d->previous_position;
        }
        p = p->get_parent();
    }
    return global_prev;
}

} // namespace RetroNode
