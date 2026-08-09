#ifndef RETRONODE_INPUT_H
#define RETRONODE_INPUT_H

#include <unordered_map>
#include <string>
#include <SDL3/SDL.h>
#include "../core/string_names.h"
#include "../core/object/class_db.h"

namespace RetroNode {

class RN_API Input {
private:
    static Input* instance;
    std::unordered_map<StringName, bool> action_states;
    std::unordered_map<SDL_Keycode, StringName> key_mappings;

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
};

} // namespace RetroNode

#endif // RETRONODE_INPUT_H
