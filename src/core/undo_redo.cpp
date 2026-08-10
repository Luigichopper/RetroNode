#include "undo_redo.h"
#include <iostream>

namespace RetroNode {

void UndoRedo::create_action(const std::string& p_name) {
    current_action = Action();
    current_action.name = p_name;
    is_building_action = true;
}

void UndoRedo::add_do_property(Object* p_target, const StringName& p_prop, const Variant& p_value) {
    if (!is_building_action || !p_target) return;
    current_action.do_ops.push_back([p_target, p_prop, p_value]() {
        p_target->set(p_prop, p_value);
    });
}

void UndoRedo::add_undo_property(Object* p_target, const StringName& p_prop, const Variant& p_value) {
    if (!is_building_action || !p_target) return;
    current_action.undo_ops.push_back([p_target, p_prop, p_value]() {
        p_target->set(p_prop, p_value);
    });
}

void UndoRedo::add_do_method(std::function<void()> p_func) {
    if (!is_building_action || !p_func) return;
    current_action.do_ops.push_back(p_func);
}

void UndoRedo::add_undo_method(std::function<void()> p_func) {
    if (!is_building_action || !p_func) return;
    current_action.undo_ops.push_back(p_func);
}

void UndoRedo::commit_action() {
    if (!is_building_action) return;
    is_building_action = false;

    // Truncate any redo history beyond action_index
    if (action_index < history.size()) {
        history.erase(history.begin() + action_index, history.end());
    }

    // Execute do_ops
    for (const auto& op : current_action.do_ops) {
        if (op) op();
    }

    history.push_back(current_action);
    action_index = history.size();
}

void UndoRedo::undo() {
    if (!has_undo()) return;
    action_index--;
    const auto& act = history[action_index];

    // Execute undo_ops in reverse order
    for (auto it = act.undo_ops.rbegin(); it != act.undo_ops.rend(); ++it) {
        if (*it) (*it)();
    }
}

void UndoRedo::redo() {
    if (!has_redo()) return;
    const auto& act = history[action_index];
    action_index++;

    // Execute do_ops in forward order
    for (const auto& op : act.do_ops) {
        if (op) op();
    }
}

const std::string& UndoRedo::get_current_action_name() const {
    static const std::string empty = "";
    if (action_index > 0 && action_index <= history.size()) {
        return history[action_index - 1].name;
    }
    return empty;
}

void UndoRedo::clear_history() {
    history.clear();
    action_index = 0;
    is_building_action = false;
}

} // namespace RetroNode
