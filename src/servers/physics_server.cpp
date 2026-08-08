#include "physics_server.h"

namespace RetroNode {

PhysicsServer2D* PhysicsServer2D::instance = nullptr;

PhysicsServer2D::PhysicsServer2D() {}

void PhysicsServer2D::add_static_box(uint64_t id, const Rect2Fixed& bounds) {
    static_bodies.push_back({id, bounds, true});
}

void PhysicsServer2D::clear() {
    static_bodies.clear();
}

Vector2Fixed PhysicsServer2D::move_and_slide(uint64_t body_id, Vector2Fixed position, Vector2Fixed size, Vector2Fixed velocity, Fixed16 delta) {
    (void)body_id;
    Vector2Fixed move_step = velocity * delta;
    Vector2Fixed new_pos = position;

    // 1. Move along X axis & check collisions
    new_pos.x += move_step.x;
    Rect2Fixed test_rect_x(new_pos, size);

    for (const auto& static_body : static_bodies) {
        if (test_rect_x.intersects(static_body.bounds)) {
            if (move_step.x > Fixed16(0)) {
                new_pos.x = static_body.bounds.position.x - size.x;
            } else if (move_step.x < Fixed16(0)) {
                new_pos.x = static_body.bounds.position.x + static_body.bounds.size.x;
            }
            break;
        }
    }

    // 2. Move along Y axis & check collisions
    new_pos.y += move_step.y;
    Rect2Fixed test_rect_y(new_pos, size);

    for (const auto& static_body : static_bodies) {
        if (test_rect_y.intersects(static_body.bounds)) {
            if (move_step.y > Fixed16(0)) {
                new_pos.y = static_body.bounds.position.y - size.y;
            } else if (move_step.y < Fixed16(0)) {
                new_pos.y = static_body.bounds.position.y + static_body.bounds.size.y;
            }
            break;
        }
    }

    return new_pos;
}

} // namespace RetroNode
