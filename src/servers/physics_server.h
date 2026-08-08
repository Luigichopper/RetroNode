#ifndef RETRONODE_PHYSICS_SERVER_H
#define RETRONODE_PHYSICS_SERVER_H

#include <vector>
#include "../core/math/vector2.h"
#include "../core/math/rect2.h"

namespace RetroNode {

struct CollisionBody {
    uint64_t id;
    Rect2Fixed bounds;
    bool is_static;
};

class PhysicsServer2D {
private:
    static PhysicsServer2D* instance;
    std::vector<CollisionBody> static_bodies;

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

    Vector2Fixed move_and_slide(uint64_t body_id, Vector2Fixed position, Vector2Fixed size, Vector2Fixed velocity, Fixed16 delta);
};

} // namespace RetroNode

#endif // RETRONODE_PHYSICS_SERVER_H
