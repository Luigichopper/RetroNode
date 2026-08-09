#ifndef RETRONODE_CHARACTER_BODY_2D_H
#define RETRONODE_CHARACTER_BODY_2D_H

#include "../2d/node_2d.h"
#include "../../core/math/vector2.h"

namespace RetroNode {

class RN_API CharacterBody2D : public Node2D {
    RN_CLASS(CharacterBody2D, Node2D)

public:
    Vector2Fixed velocity;
    Vector2Fixed body_size;
    Fixed16 last_delta;

    CharacterBody2D();
    virtual ~CharacterBody2D() = default;

    void move_and_slide();
    virtual void _physics_process(Fixed16 delta) override;
};

} // namespace RetroNode

#endif // RETRONODE_CHARACTER_BODY_2D_H
