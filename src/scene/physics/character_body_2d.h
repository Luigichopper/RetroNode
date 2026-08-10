#ifndef RETRONODE_CHARACTER_BODY_2D_H
#define RETRONODE_CHARACTER_BODY_2D_H

#include "../2d/node_2d.h"
#include "../../core/math/vector2.h"
#include "collision_shape_2d.h"
#include <vector>

namespace RetroNode {

class RN_API CharacterBody2D : public Node2D {
    RN_CLASS(CharacterBody2D, Node2D)

private:
    std::vector<CollisionShape2D*> collision_shapes;

public:
    Vector2Fixed velocity;
    Vector2Fixed body_size;
    Fixed16 last_delta;

    void set_velocity(const Vector2Fixed& v) { velocity = v; }
    Vector2Fixed get_velocity() const { return velocity; }

    void set_body_size(const Vector2Fixed& s) { body_size = s; }
    Vector2Fixed get_body_size() const { return body_size; }

    CharacterBody2D();
    virtual ~CharacterBody2D() = default;

    virtual void _ready() override;
    void update_shapes();
    const std::vector<CollisionShape2D*>& get_collision_shapes() const { return collision_shapes; }

    void move_and_slide();
    virtual void _physics_process(Fixed16 delta) override;
};

} // namespace RetroNode

#endif // RETRONODE_CHARACTER_BODY_2D_H

