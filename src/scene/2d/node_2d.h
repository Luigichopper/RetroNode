#ifndef RETRONODE_NODE_2D_H
#define RETRONODE_NODE_2D_H

#include "../main/node.h"
#include "../../core/math/vector2.h"
#include "../../core/math/rect2.h"

namespace RetroNode {

class Node2D : public Node {
    RN_CLASS(Node2D, Node)

public:
    Vector2Fixed position;
    Fixed16 rotation;
    Vector2Fixed scale;

    Node2D();
    virtual ~Node2D() = default;

    Vector2Fixed get_global_position() const;
    void set_position(const Vector2Fixed& pos) { position = pos; }
    Vector2Fixed get_position() const { return position; }
};

} // namespace RetroNode

#endif // RETRONODE_NODE_2D_H
