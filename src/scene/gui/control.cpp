#include "control.h"

namespace RetroNode {

Control::Control()
    : position(Vector2Fixed::zero()),
      previous_position(Vector2Fixed::zero()),
      size(Vector2Fixed::from_floats(40.0f, 40.0f)) {
    name = "Control";
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
