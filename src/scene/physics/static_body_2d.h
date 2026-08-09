#ifndef RETRONODE_STATIC_BODY_2D_H
#define RETRONODE_STATIC_BODY_2D_H

#include "../2d/node_2d.h"
#include "collision_shape_2d.h"
#include <vector>

namespace RetroNode {

class RN_API StaticBody2D : public Node2D {
    RN_CLASS(StaticBody2D, Node2D)

private:
    std::vector<CollisionShape2D*> collision_shapes;

public:
    StaticBody2D();
    ~StaticBody2D() override = default;

    virtual void _ready() override;
    void update_shapes();
    void register_shapes();
    const std::vector<CollisionShape2D*>& get_collision_shapes() const { return collision_shapes; }
};

} // namespace RetroNode

#endif // RETRONODE_STATIC_BODY_2D_H
