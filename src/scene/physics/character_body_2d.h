#ifndef RETRONODE_CHARACTER_BODY_2D_H
#define RETRONODE_CHARACTER_BODY_2D_H

#include "../2d/node_2d.h"
#include "../../core/math/vector2.h"
#include "../../servers/physics_server.h"
#include "collision_shape_2d.h"
#include <vector>

namespace RetroNode {

class RN_API CharacterBody2D : public Node2D {
    RN_CLASS(CharacterBody2D, Node2D)

private:
    std::vector<CollisionShape2D*> collision_shapes;
    bool on_floor = false;
    bool on_ceiling = false;
    bool on_wall = false;
    KinematicCollision2D last_collision;

public:
    Vector2Fixed velocity;
    Vector2Fixed body_size;
    Fixed16 last_delta;

    void set_velocity(const Vector2Fixed& v) { velocity = v; }
    Vector2Fixed get_velocity() const { return velocity; }

    void set_body_size(const Vector2Fixed& s) { body_size = s; }
    Vector2Fixed get_body_size() const { return body_size; }

    bool is_on_floor() const { return on_floor; }
    bool is_on_ceiling() const { return on_ceiling; }
    bool is_on_wall() const { return on_wall; }
    KinematicCollision2D get_last_slide_collision() const { return last_collision; }

    CharacterBody2D();
    virtual ~CharacterBody2D() override;

    virtual void _ready() override;
    void update_shapes();
    const std::vector<CollisionShape2D*>& get_collision_shapes() const { return collision_shapes; }

    void move_and_slide();
    virtual void _physics_process(Fixed16 delta) override;
};

} // namespace RetroNode

#endif // RETRONODE_CHARACTER_BODY_2D_H

