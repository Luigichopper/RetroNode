#include "input.h"
#include <algorithm>
#include <iostream>

namespace RetroNode {

Input* Input::instance = nullptr;

Input::Input() {
    // Default actions and key bindings
    add_action("ui_left");
    bind_action(SDLK_A, "ui_left");
    bind_action(SDLK_LEFT, "ui_left");

    add_action("ui_right");
    bind_action(SDLK_D, "ui_right");
    bind_action(SDLK_RIGHT, "ui_right");

    add_action("ui_up");
    bind_action(SDLK_W, "ui_up");
    bind_action(SDLK_UP, "ui_up");

    add_action("ui_down");
    bind_action(SDLK_S, "ui_down");
    bind_action(SDLK_DOWN, "ui_down");

    add_action("ui_accept");
    bind_action(SDLK_SPACE, "ui_accept");
    bind_action(SDLK_RETURN, "ui_accept");

    add_action("action_jump");
    bind_action(SDLK_SPACE, "action_jump");
    bind_action(SDLK_Z, "action_jump");

    add_action("action_run");
    bind_action(SDLK_LSHIFT, "action_run");
    bind_action(SDLK_X, "action_run");

    add_action("action_attack");
    bind_action(SDLK_J, "action_attack");
    bind_action(SDLK_X, "action_attack");
}

void Input::handle_event(const SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
        bool is_pressed = (event.type == SDL_EVENT_KEY_DOWN);
        auto range = key_mappings.equal_range(event.key.key);
        for (auto it = range.first; it != range.second; ++it) {
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

void Input::add_action(const StringName& action) {
    if (action.as_string().empty()) return;
    if (std::find(actions_order.begin(), actions_order.end(), action) == actions_order.end()) {
        actions_order.push_back(action);
    }
}

void Input::remove_action(const StringName& action) {
    actions_order.erase(std::remove(actions_order.begin(), actions_order.end(), action), actions_order.end());
    action_states.erase(action);

    for (auto it = key_mappings.begin(); it != key_mappings.end(); ) {
        if (it->second == action) {
            it = key_mappings.erase(it);
        } else {
            ++it;
        }
    }
}

void Input::bind_action(SDL_Keycode key, const StringName& action) {
    add_action(action);
    // Avoid duplicate key-action pair
    auto range = key_mappings.equal_range(key);
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second == action) return;
    }
    key_mappings.insert({key, action});
}

void Input::unbind_action(SDL_Keycode key, const StringName& action) {
    auto range = key_mappings.equal_range(key);
    for (auto it = range.first; it != range.second; ) {
        if (it->second == action) {
            it = key_mappings.erase(it);
        } else {
            ++it;
        }
    }
}

std::vector<StringName> Input::get_action_list() const {
    return actions_order;
}

std::vector<SDL_Keycode> Input::get_action_keys(const StringName& action) const {
    std::vector<SDL_Keycode> keys;
    for (const auto& pair : key_mappings) {
        if (pair.second == action) {
            keys.push_back(pair.first);
        }
    }
    return keys;
}

void Input::load_from_json(const nlohmann::json& j) {
    if (!j.is_object()) return;
    actions_order.clear();
    key_mappings.clear();
    action_states.clear();

    for (auto& [action_name, keys_arr] : j.items()) {
        StringName act(action_name);
        add_action(act);
        if (keys_arr.is_array()) {
            for (const auto& k : keys_arr) {
                if (k.is_number_integer()) {
                    bind_action(static_cast<SDL_Keycode>(k.get<int>()), act);
                } else if (k.is_string()) {
                    SDL_Keycode code = SDL_GetKeyFromName(k.get<std::string>().c_str());
                    if (code != SDLK_UNKNOWN) {
                        bind_action(code, act);
                    }
                }
            }
        }
    }
    std::cout << "[Input] Loaded " << actions_order.size() << " custom actions from project settings." << std::endl;
}

nlohmann::json Input::save_to_json() const {
    nlohmann::json j = nlohmann::json::object();
    for (const auto& act : actions_order) {
        std::string act_name = act.as_string();
        j[act_name] = nlohmann::json::array();
        std::vector<SDL_Keycode> keys = get_action_keys(act);
        for (SDL_Keycode k : keys) {
            const char* key_name = SDL_GetKeyName(k);
            if (key_name && key_name[0] != '\0') {
                j[act_name].push_back(std::string(key_name));
            } else {
                j[act_name].push_back(static_cast<int>(k));
            }
        }
    }
    return j;
}

} // namespace RetroNode
