#include "player_controller.h"
#include <iostream>

PlayerController::PlayerController() { name = "PlayerController"; }

void PlayerController::_ready() {
  std::cout << "[MyRPG] PlayerController ready!" << std::endl;
  sprite = get_node<AnimatedSprite2D>("Sprite");
  hurt_audio = get_node<AudioStreamPlayer>("HurtAudio");
  if (hurt_audio) {
    std::cout << "[MyRPG] PlayerController successfully bound HurtAudio node." << std::endl;
  } else {
    std::cout << "[MyRPG] Warning: HurtAudio node not found on PlayerController!" << std::endl;
  }
}

void PlayerController::_physics_process(Fixed16 delta) {
  CharacterBody2D::_physics_process(delta);

  Fixed16 vel_x(0);
  Fixed16 vel_y(0);

  if (Input::get()->is_action_pressed(StringName("ui_right")))
    vel_x = move_speed;
  if (Input::get()->is_action_pressed(StringName("ui_left")))
    vel_x = -move_speed;
  if (Input::get()->is_action_pressed(StringName("ui_down")))
    vel_y = move_speed;
  if (Input::get()->is_action_pressed(StringName("ui_up")))
    vel_y = -move_speed;

  bool accept_pressed = Input::get()->is_action_pressed(StringName("ui_accept")) ||
                         Input::get()->is_action_pressed(StringName("action_attack"));

  if (accept_pressed && !was_accept_pressed) {
    std::cout << "[MyRPG] Spacebar pressed! Triggering hurt_audio..." << std::endl;
    if (hurt_audio) {
      hurt_audio->play();
    }
  }
  was_accept_pressed = accept_pressed;

  this->velocity.x = vel_x;
  this->velocity.y = vel_y;

  this->move_and_slide();
  update_animation();
}

void PlayerController::update_animation() {
  if (!sprite)
    return;

  if (this->velocity.x.raw > 0)
    sprite->play("walk_right");
  else if (this->velocity.x.raw < 0)
    sprite->play("walk_left");
  else if (this->velocity.y.raw > 0)
    sprite->play("walk_down");
  else if (this->velocity.y.raw < 0)
    sprite->play("walk_up");
  else
    sprite->play("idle_down");
}

RN_REGISTER_CLASS(PlayerController);
