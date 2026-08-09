#ifndef RETRONODE_AREA_2D_H
#define RETRONODE_AREA_2D_H

#include "../2d/node_2d.h"
#include "collision_shape_2d.h"
#include <vector>
#include <functional>

namespace RetroNode {

class RN_API Area2D : public Node2D {
    RN_CLASS(Area2D, Node2D)

private:
    std::vector<CollisionShape2D*> collision_shapes;
    std::vector<Node2D*> overlapping_nodes;

public:
    Area2D();
    ~Area2D() override = default;

    virtual void _ready() override;
    virtual void _physics_process(Fixed16 delta) override;

    void update_shapes();
    const std::vector<CollisionShape2D*>& get_collision_shapes() const { return collision_shapes; }
    const std::vector<Node2D*>& get_overlapping_nodes() const { return overlapping_nodes; }

    std::function<void(Node2D*)> on_body_entered;
    std::function<void(Node2D*)> on_body_exited;

    void notify_body_entered(Node2D* p_node);
    void notify_body_exited(Node2D* p_node);
};

} // namespace RetroNode

#endif // RETRONODE_AREA_2D_H
