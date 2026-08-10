#ifndef RETRONODE_UNDO_REDO_H
#define RETRONODE_UNDO_REDO_H

#include "object/object.h"
#include "variant.h"
#include "string_names.h"
#include <string>
#include <vector>
#include <functional>

namespace RetroNode {

class RN_API UndoRedo {
public:
    struct Action {
        std::string name;
        std::vector<std::function<void()>> do_ops;
        std::vector<std::function<void()>> undo_ops;
    };

private:
    std::vector<Action> history;
    size_t action_index = 0;
    Action current_action;
    bool is_building_action = false;

public:
    UndoRedo() = default;
    ~UndoRedo() = default;

    void create_action(const std::string& p_name);
    void add_do_property(Object* p_target, const StringName& p_prop, const Variant& p_value);
    void add_undo_property(Object* p_target, const StringName& p_prop, const Variant& p_value);
    void add_do_method(std::function<void()> p_func);
    void add_undo_method(std::function<void()> p_func);
    void commit_action();

    void undo();
    void redo();

    bool has_undo() const noexcept { return action_index > 0; }
    bool has_redo() const noexcept { return action_index < history.size(); }
    size_t get_history_count() const noexcept { return history.size(); }
    const std::string& get_current_action_name() const;

    void clear_history();
};

} // namespace RetroNode

#endif // RETRONODE_UNDO_REDO_H
