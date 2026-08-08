#include "player_controller.h"
#include <iostream>

PlayerController::PlayerController() {
    name = "PlayerController";
}

void PlayerController::_ready() {
    std::cout << "[MyRPG] PlayerController ready!" << std::endl;
    sprite = get_node<AnimatedSprite2D>("Sprite");
}

void PlayerController::_physics_process(Fixed16 delta) {
    CharacterBody2D::_physics_process(delta);

    Fixed16 vel_x(0);
    Fixed16 vel_y(0);

    if (Input::get()->is_action_pressed(StringName("ui_right"))) vel_x = move_speed;
    if (Input::get()->is_action_pressed(StringName("ui_left")))  vel_x = move_speed * Fixed16(-1);
    if (Input::get()->is_action_pressed(StringName("ui_down")))  vel_y = move_speed;
    if (Input::get()->is_action_pressed(StringName("ui_up")))    vel_y = move_speed * Fixed16(-1);

    this->velocity.x = vel_x;
    this->velocity.y = vel_y;

    this->move_and_slide();
    update_animation();
}

void PlayerController::update_animation() {
    if (!sprite) return;

    if (this->velocity.x.raw > 0) sprite->play("walk_right");
    else if (this->velocity.x.raw < 0) sprite->play("walk_left");
    else if (this->velocity.y.raw > 0) sprite->play("walk_down");
    else if (this->velocity.y.raw < 0) sprite->play("walk_up");
    else sprite->play("idle_down");
}

RN_REGISTER_CLASS(PlayerController);
