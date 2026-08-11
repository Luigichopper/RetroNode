#ifndef RETRONODE_INPUT_H
#define RETRONODE_INPUT_H

#include <unordered_map>
#include <vector>
#include <string>
#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>
#include "../core/string_names.h"
#include "../core/object/class_db.h"

namespace RetroNode {

class RN_API Input {
private:
    static Input* instance;
    std::unordered_map<StringName, bool> action_states;
    std::unordered_map<SDL_Keycode, StringName> key_mappings;
    std::vector<StringName> actions_order;

public:
    Input();

    static Input* get() {
        if (!instance) {
            instance = new Input();
        }
        return instance;
    }

    void handle_event(const SDL_Event& event);
    bool is_action_pressed(const StringName& action) const;
    void set_action_state(const StringName& action, bool pressed);

    void add_action(const StringName& action);
    void remove_action(const StringName& action);
    void bind_action(SDL_Keycode key, const StringName& action);
    void unbind_action(SDL_Keycode key, const StringName& action);

    std::vector<StringName> get_action_list() const;
    std::vector<SDL_Keycode> get_action_keys(const StringName& action) const;

    void load_from_json(const nlohmann::json& j);
    nlohmann::json save_to_json() const;
};

} // namespace RetroNode

#endif // RETRONODE_INPUT_H
