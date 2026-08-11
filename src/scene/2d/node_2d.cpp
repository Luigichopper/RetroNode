#include "node_2d.h"

namespace RetroNode {

Node2D::Node2D() 
    : position(Vector2Fixed::zero()), 
      previous_position(Vector2Fixed::zero()),
      rotation(0), 
      scale(Vector2Fixed::from_floats(1.0f, 1.0f)) {
    name = "Node2D";
}

void Node2D::get_property_list(std::vector<PropertyInfo>& out_list) const {
    Node::get_property_list(out_list);
    out_list.push_back({ StringName("position"), VariantType::VECTOR2 });
    out_list.push_back({ StringName("rotation"), VariantType::FLOAT16 });
    out_list.push_back({ StringName("scale"), VariantType::VECTOR2 });
}

Variant Node2D::get(const StringName& p_name) const {
    static const StringName s_position("position");
    static const StringName s_rotation("rotation");
    static const StringName s_scale("scale");

    if (p_name == s_position) return Variant(position);
    if (p_name == s_rotation) return Variant(rotation);
    if (p_name == s_scale) return Variant(scale);
    return Node::get(p_name);
}

bool Node2D::set(const StringName& p_name, const Variant& p_value) {
    static const StringName s_position("position");
    static const StringName s_rotation("rotation");
    static const StringName s_scale("scale");

    if (p_name == s_position) {
        set_position(p_value.as_vector2());
        return true;
    }
    if (p_name == s_rotation) {
        rotation = p_value.as_fixed16();
        return true;
    }
    if (p_name == s_scale) {
        scale = p_value.as_vector2();
        return true;
    }
    return Node::set(p_name, p_value);
}

#include <cmath>

Vector2Fixed Node2D::get_global_position() const {
    Vector2Fixed global_pos = position;
    const Node* p = get_parent();
    while (p) {
        const Node2D* n2d = dynamic_cast<const Node2D*>(p);
        if (n2d) {
            // 1. Scale
            global_pos.x *= n2d->scale.x;
            global_pos.y *= n2d->scale.y;

            // 2. Rotate
            if (n2d->rotation.raw != 0) {
                float rad = n2d->rotation.to_float();
                float cos_a = std::cos(rad);
                float sin_a = std::sin(rad);
                float rx = global_pos.x.to_float() * cos_a - global_pos.y.to_float() * sin_a;
                float ry = global_pos.x.to_float() * sin_a + global_pos.y.to_float() * cos_a;
                global_pos = Vector2Fixed::from_floats(rx, ry);
            }

            // 3. Translate
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
            // 1. Scale
            global_prev.x *= n2d->scale.x;
            global_prev.y *= n2d->scale.y;

            // 2. Rotate
            if (n2d->rotation.raw != 0) {
                float rad = n2d->rotation.to_float();
                float cos_a = std::cos(rad);
                float sin_a = std::sin(rad);
                float rx = global_prev.x.to_float() * cos_a - global_prev.y.to_float() * sin_a;
                float ry = global_prev.x.to_float() * sin_a + global_prev.y.to_float() * cos_a;
                global_prev = Vector2Fixed::from_floats(rx, ry);
            }

            // 3. Translate
            global_prev += n2d->previous_position;
        }
        p = p->get_parent();
    }
    return global_prev;
}

Fixed16 Node2D::get_global_rotation() const {
    Fixed16 global_rot = rotation;
    const Node* p = get_parent();
    while (p) {
        const Node2D* n2d = dynamic_cast<const Node2D*>(p);
        if (n2d) {
            global_rot += n2d->rotation;
        }
        p = p->get_parent();
    }
    return global_rot;
}

Vector2Fixed Node2D::get_global_scale() const {
    Vector2Fixed global_scale = scale;
    const Node* p = get_parent();
    while (p) {
        const Node2D* n2d = dynamic_cast<const Node2D*>(p);
        if (n2d) {
            global_scale.x *= n2d->scale.x;
            global_scale.y *= n2d->scale.y;
        }
        p = p->get_parent();
    }
    return global_scale;
}

} // namespace RetroNode
