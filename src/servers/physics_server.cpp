#include "physics_server.h"
#include <algorithm>
#include <cmath>

namespace RetroNode {

PhysicsServer2D* PhysicsServer2D::instance = nullptr;

PhysicsServer2D::PhysicsServer2D() {}

static inline int to_cell(float coord, int cell_sz) {
    return static_cast<int>(std::floor(coord / static_cast<float>(cell_sz)));
}

void PhysicsServer2D::add_static_box(uint64_t id, const Rect2Fixed& bounds) {
    size_t index = static_bodies.size();
    static_bodies.push_back({id, bounds, true});

    float x1 = bounds.position.x.to_float();
    float y1 = bounds.position.y.to_float();
    float x2 = x1 + bounds.size.x.to_float() - 0.01f;
    float y2 = y1 + bounds.size.y.to_float() - 0.01f;

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

void PhysicsServer2D::clear() {
    static_bodies.clear();
    spatial_grid.clear();
}

std::vector<size_t> PhysicsServer2D::get_nearby_body_indices(const Rect2Fixed& bounds) const {
    std::vector<size_t> nearby;
    std::unordered_set<size_t> visited;

    float x1 = bounds.position.x.to_float();
    float y1 = bounds.position.y.to_float();
    float x2 = x1 + bounds.size.x.to_float() - 0.01f;
    float y2 = y1 + bounds.size.y.to_float() - 0.01f;

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
                    if (visited.insert(idx).second) {
                        nearby.push_back(idx);
                    }
                }
            }
        }
    }
    return nearby;
}

Vector2Fixed PhysicsServer2D::move_and_slide(uint64_t body_id, Vector2Fixed position, Vector2Fixed size, Vector2Fixed velocity, Fixed16 delta) {
    (void)body_id;
    Vector2Fixed move_step = velocity * delta;
    Vector2Fixed new_pos = position;

    // 1. Move along X axis & check spatial grid cell collisions
    new_pos.x += move_step.x;
    Rect2Fixed test_rect_x(new_pos, size);
    std::vector<size_t> nearby_x = get_nearby_body_indices(test_rect_x);

    for (size_t idx : nearby_x) {
        const auto& static_body = static_bodies[idx];
        if (test_rect_x.intersects(static_body.bounds)) {
            if (move_step.x > Fixed16(0)) {
                new_pos.x = static_body.bounds.position.x - size.x;
            } else if (move_step.x < Fixed16(0)) {
                new_pos.x = static_body.bounds.position.x + static_body.bounds.size.x;
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
