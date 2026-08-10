#include "control.h"

namespace RetroNode {

Control::Control()
    : position(Vector2Fixed::zero()),
      previous_position(Vector2Fixed::zero()),
      size(Vector2Fixed::from_floats(40.0f, 40.0f)) {
    name = "Control";
}

void Control::get_property_list(std::vector<PropertyInfo>& out_list) const {
    Node::get_property_list(out_list);
    out_list.push_back({ StringName("position"), VariantType::VECTOR2 });
    out_list.push_back({ StringName("size"), VariantType::VECTOR2 });
    out_list.push_back({ StringName("z_index"), VariantType::INT });
}

Variant Control::get(const StringName& p_name) const {
    if (p_name == StringName("position")) return Variant(position);
    if (p_name == StringName("size")) return Variant(size);
    if (p_name == StringName("z_index")) return Variant((int64_t)z_index);
    return Node::get(p_name);
}

bool Control::set(const StringName& p_name, const Variant& p_value) {
    if (p_name == StringName("position")) {
        set_position(p_value.as_vector2());
        return true;
    }
    if (p_name == StringName("size")) {
        set_size(p_value.as_vector2());
        return true;
    }
    if (p_name == StringName("z_index")) {
        z_index = static_cast<int>(p_value.as_int());
        return true;
    }
    return Node::set(p_name, p_value);
}

Vector2Fixed Control::get_global_control_position() const {
    Vector2Fixed global_pos = position;
    const Node* p = get_parent();
    while (p) {
        const Control* ctrl = dynamic_cast<const Control*>(p);
        if (ctrl) {
            global_pos += ctrl->position;
        }
        p = p->get_parent();
    }
    return global_pos;
}

Vector2Fixed Control::get_global_control_previous_position() const {
    Vector2Fixed global_prev = previous_position;
    const Node* p = get_parent();
    while (p) {
        const Control* ctrl = dynamic_cast<const Control*>(p);
        if (ctrl) {
            global_prev += ctrl->previous_position;
        }
        p = p->get_parent();
    }
    return global_prev;
}

} // namespace RetroNode
