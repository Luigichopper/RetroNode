#include "input.h"

namespace RetroNode {

Input* Input::instance = nullptr;

Input::Input() {
    // Default WASD & Arrow Key mappings
    key_mappings[SDLK_A] = StringName("ui_left");
    key_mappings[SDLK_LEFT] = StringName("ui_left");

    key_mappings[SDLK_D] = StringName("ui_right");
    key_mappings[SDLK_RIGHT] = StringName("ui_right");

    key_mappings[SDLK_W] = StringName("ui_up");
    key_mappings[SDLK_UP] = StringName("ui_up");

    key_mappings[SDLK_S] = StringName("ui_down");
    key_mappings[SDLK_DOWN] = StringName("ui_down");

    key_mappings[SDLK_SPACE] = StringName("ui_accept");
    key_mappings[SDLK_RETURN] = StringName("ui_accept");

    // Game action & debug overlay toggles
    key_mappings[SDLK_F3] = StringName("toggle_debug");
    key_mappings[SDLK_Z] = StringName("action_attack");
    key_mappings[SDLK_J] = StringName("action_attack");
}

void Input::handle_event(const SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
        bool is_pressed = (event.type == SDL_EVENT_KEY_DOWN);
        auto it = key_mappings.find(event.key.key);
        if (it != key_mappings.end()) {
            action_states[it->second] = is_pressed;
        }
    }
}

bool Input::is_action_pressed(const StringName& action) const {
    auto it = action_states.find(action);
    if (it != action_states.end()) {
        return it->second;
    }
    return false;
}

void Input::set_action_state(const StringName& action, bool pressed) {
    action_states[action] = pressed;
}

void Input::bind_action(SDL_Keycode key, const StringName& action) {
    key_mappings[key] = action;
}

} // namespace RetroNode
