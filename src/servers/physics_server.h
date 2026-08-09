#ifndef RETRONODE_PHYSICS_SERVER_H
#define RETRONODE_PHYSICS_SERVER_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "../core/math/vector2.h"
#include "../core/math/rect2.h"
#include "../core/object/class_db.h"

namespace RetroNode {

struct CollisionBody {
    uint64_t id;
    Rect2Fixed bounds;
    bool is_static;
};

class RN_API PhysicsServer2D {
private:
    static PhysicsServer2D* instance;
    std::vector<CollisionBody> static_bodies;
    
    // Spatial Hash Grid partitioning for O(1) cell lookups
    static constexpr int CELL_SIZE = 32;
    std::unordered_map<uint64_t, std::vector<size_t>> spatial_grid;

    uint64_t get_cell_key(int grid_x, int grid_y) const {
        return (static_cast<uint64_t>(grid_x) << 32) | (static_cast<uint32_t>(grid_y) & 0xFFFFFFFF);
    }

public:
    PhysicsServer2D();

    static PhysicsServer2D* get() {
        if (!instance) {
            instance = new PhysicsServer2D();
        }
        return instance;
    }

    void add_static_box(uint64_t id, const Rect2Fixed& bounds);
    void clear();

    std::vector<size_t> get_nearby_body_indices(const Rect2Fixed& bounds) const;

    Vector2Fixed move_and_slide(uint64_t body_id, Vector2Fixed position, Vector2Fixed size, Vector2Fixed velocity, Fixed16 delta);
};

} // namespace RetroNode

#endif // RETRONODE_PHYSICS_SERVER_H
