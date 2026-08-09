#include "character_body_2d.h"
#include "../../servers/physics_server.h"

namespace RetroNode {

CharacterBody2D::CharacterBody2D()
    : velocity(Vector2Fixed::zero()),
      body_size(Vector2Fixed::from_floats(16.0f, 16.0f)),
      last_delta(Fixed16::from_float(1.0f / 60.0f)) {
    name = "CharacterBody2D";
}

void CharacterBody2D::_physics_process(Fixed16 delta) {
    Node2D::_physics_process(delta);
    last_delta = delta;
}

void CharacterBody2D::move_and_slide() {
    Vector2Fixed new_pos = PhysicsServer2D::get()->move_and_slide(
        get_instance_id(),
        position,
        body_size,
        velocity,
        last_delta
    );
    position = new_pos;
}

} // namespace RetroNode
