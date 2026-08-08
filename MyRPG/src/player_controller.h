#ifndef MYRPG_PLAYER_CONTROLLER_H
#define MYRPG_PLAYER_CONTROLLER_H

#include "scene/physics/character_body_2d.h"
#include "scene/2d/sprite_2d.h"
#include "servers/input.h"
#include "core/object/class_db.h"

using namespace RetroNode;

class PlayerController : public CharacterBody2D {
    RN_CLASS(PlayerController, CharacterBody2D)

private:
    Fixed16 move_speed = Fixed16::from_float(90.0f);
    AnimatedSprite2D* sprite = nullptr;

public:
    PlayerController();
    virtual ~PlayerController() = default;

    virtual void _ready() override;
    virtual void _physics_process(Fixed16 delta) override;
    void update_animation();
};

#endif // MYRPG_PLAYER_CONTROLLER_H
