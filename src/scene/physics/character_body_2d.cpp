#include "character_body_2d.h"
#include "../../servers/physics_server.h"

namespace RetroNode {

CharacterBody2D::CharacterBody2D()
    : velocity(Vector2Fixed::zero()),
      body_size(Vector2Fixed::from_floats(16.0f, 16.0f)),
      last_delta(Fixed16::from_float(1.0f / 60.0f)) {
    name = "CharacterBody2D";
}

void CharacterBody2D::_ready() {
    Node2D::_ready();
    update_shapes();
}

void CharacterBody2D::update_shapes() {
    collision_shapes.clear();
    for (Node* child : get_children()) {
        if (CollisionShape2D* shape = dynamic_cast<CollisionShape2D*>(child)) {
            collision_shapes.push_back(shape);
        }
    }
}

void CharacterBody2D::_physics_process(Fixed16 delta) {
    Node2D::_physics_process(delta);
    last_delta = delta;
}

void CharacterBody2D::move_and_slide() {
    if (collision_shapes.empty()) {
        update_shapes();
    }

    Vector2Fixed effective_size = body_size;
    if (!collision_shapes.empty() && collision_shapes[0] != nullptr) {
        Rect2Fixed box = collision_shapes[0]->get_global_box();
        effective_size = box.size;
    }

    Vector2Fixed new_pos = PhysicsServer2D::get()->move_and_slide(
        get_instance_id(),
        position,
        effective_size,
        velocity,
        last_delta
    );
    position = new_pos;

    // Register active body bounds with PhysicsServer2D for Area2D triggers
    Rect2Fixed current_bounds(position, effective_size);
    PhysicsServer2D::get()->register_active_body(get_instance_id(), this, current_bounds);
}

} // namespace RetroNode
