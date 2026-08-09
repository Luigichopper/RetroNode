#ifndef RETRONODE_NODE_2D_H
#define RETRONODE_NODE_2D_H

#include "../main/node.h"
#include "../../core/math/vector2.h"
#include "../../core/math/rect2.h"

namespace RetroNode {

class RN_API Node2D : public Node {
    RN_CLASS(Node2D, Node)

public:
    Vector2Fixed position;
    Vector2Fixed previous_position;
    Fixed16 rotation;
    Vector2Fixed scale;

    Node2D();
    virtual ~Node2D() = default;

    Vector2Fixed get_global_position() const;
    Vector2Fixed get_global_previous_position() const;

    void set_position(const Vector2Fixed& pos) { 
        previous_position = position;
        position = pos; 
    }
    Vector2Fixed get_position() const { return position; }

    virtual void _physics_process(Fixed16 delta) override {
        previous_position = position;
        Node::_physics_process(delta);
    }
};

} // namespace RetroNode

#endif // RETRONODE_NODE_2D_H
