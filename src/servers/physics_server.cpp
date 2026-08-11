#include "physics_server.h"
#include "../scene/2d/node_2d.h"
#include <algorithm>
#include <cmath>

namespace RetroNode {

PhysicsServer2D* PhysicsServer2D::instance = nullptr;

PhysicsServer2D::PhysicsServer2D() {}

static inline int to_cell(Fixed16 coord, int cell_sz) {
    int c = coord.to_int();
    return (c >= 0) ? (c / cell_sz) : ((c - cell_sz + 1) / cell_sz);
}

void PhysicsServer2D::add_static_box(uint64_t id, const Rect2Fixed& bounds) {
    size_t index = static_bodies.size();
    static_bodies.push_back({id, bounds, true});

    Fixed16 epsilon = Fixed16::from_raw(1);
    Fixed16 x1 = bounds.position.x;
    Fixed16 y1 = bounds.position.y;
    Fixed16 x2 = bounds.position.x + bounds.size.x - epsilon;
    Fixed16 y2 = bounds.position.y + bounds.size.y - epsilon;

    int min_gx = to_cell(x1, CELL_SIZE);
    int max_gx = to_cell(x2, CELL_SIZE);
    int min_gy = to_cell(y1, CELL_SIZE);
    int max_gy = to_cell(y2, CELL_SIZE);

    for (int gx = min_gx; gx <= max_gx; ++gx) {
        for (int gy = min_gy; gy <= max_gy; ++gy) {
            uint64_t key = get_cell_key(gx, gy);
            spatial_grid[key].push_back(index);
        }
    }
}

void PhysicsServer2D::register_active_body(uint64_t id, Node2D* node, const Rect2Fixed& bounds) {
    active_bodies[id] = ActiveBodyInfo{id, node, bounds};
}

void PhysicsServer2D::unregister_active_body(uint64_t id) {
    active_bodies.erase(id);
}

void PhysicsServer2D::clear() {
    static_bodies.clear();
    active_bodies.clear();
    spatial_grid.clear();
}

std::vector<size_t> PhysicsServer2D::get_nearby_body_indices(const Rect2Fixed& bounds) const {
    std::vector<size_t> nearby;
    nearby.reserve(16);

    Fixed16 epsilon = Fixed16::from_raw(1);
    Fixed16 x1 = bounds.position.x;
    Fixed16 y1 = bounds.position.y;
    Fixed16 x2 = bounds.position.x + bounds.size.x - epsilon;
    Fixed16 y2 = bounds.position.y + bounds.size.y - epsilon;

    int min_gx = to_cell(x1, CELL_SIZE);
    int max_gx = to_cell(x2, CELL_SIZE);
    int min_gy = to_cell(y1, CELL_SIZE);
    int max_gy = to_cell(y2, CELL_SIZE);

    for (int gx = min_gx; gx <= max_gx; ++gx) {
        for (int gy = min_gy; gy <= max_gy; ++gy) {
            uint64_t key = get_cell_key(gx, gy);
            auto it = spatial_grid.find(key);
            if (it != spatial_grid.end()) {
                for (size_t idx : it->second) {
                    if (std::find(nearby.begin(), nearby.end(), idx) == nearby.end()) {
                        nearby.push_back(idx);
                    }
                }
            }
        }
    }
    return nearby;
}

std::vector<Node2D*> PhysicsServer2D::get_overlapping_bodies_for_box(const Rect2Fixed& bounds, Node2D* self) const {
    std::vector<Node2D*> result;
    for (const auto& pair : active_bodies) {
        const auto& info = pair.second;
        if (!info.node || info.node == self) continue;
        if (bounds.intersects(info.bounds)) {
            result.push_back(info.node);
        }
    }
    return result;
}

KinematicCollision2D PhysicsServer2D::move_and_slide(uint64_t body_id, Vector2Fixed position, Vector2Fixed size, Vector2Fixed velocity, Fixed16 delta) {
    (void)body_id;
    KinematicCollision2D result;
    result.new_position = position;

    Vector2Fixed move_step = velocity * delta;
    Vector2Fixed new_pos = position;

    // 1. Move along X axis & check spatial grid cell collisions
    new_pos.x += move_step.x;
    Rect2Fixed test_rect_x(new_pos, size);
    std::vector<size_t> nearby_x = get_nearby_body_indices(test_rect_x);

    for (size_t idx : nearby_x) {
        const auto& static_body = static_bodies[idx];
        if (test_rect_x.intersects(static_body.bounds)) {
            result.collided = true;
            result.on_wall = true;
            result.collided_body_id = static_body.id;

            if (move_step.x > Fixed16(0)) {
                new_pos.x = static_body.bounds.position.x - size.x;
                result.normal = Vector2Fixed::from_floats(-1.0f, 0.0f);
            } else if (move_step.x < Fixed16(0)) {
                new_pos.x = static_body.bounds.position.x + static_body.bounds.size.x;
                result.normal = Vector2Fixed::from_floats(1.0f, 0.0f);
            }
            break;
        }
    }

    // 2. Move along Y axis & check spatial grid cell collisions
    new_pos.y += move_step.y;
    Rect2Fixed test_rect_y(new_pos, size);
    std::vector<size_t> nearby_y = get_nearby_body_indices(test_rect_y);

    for (size_t idx : nearby_y) {
        const auto& static_body = static_bodies[idx];
        if (test_rect_y.intersects(static_body.bounds)) {
            result.collided = true;
            result.collided_body_id = static_body.id;

            if (move_step.y > Fixed16(0)) {
                // Moving down -> Landed on floor
                new_pos.y = static_body.bounds.position.y - size.y;
                result.on_floor = true;
                result.normal = Vector2Fixed::from_floats(0.0f, -1.0f);
            } else if (move_step.y < Fixed16(0)) {
                // Moving up -> Hit ceiling
                new_pos.y = static_body.bounds.position.y + static_body.bounds.size.y;
                result.on_ceiling = true;
                result.normal = Vector2Fixed::from_floats(0.0f, 1.0f);
            }
            break;
        }
    }

    result.new_position = new_pos;
    return result;
}

} // namespace RetroNode
