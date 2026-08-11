#ifndef RETRONODE_EDITOR_STATE_H
#define RETRONODE_EDITOR_STATE_H

#include "../core/object/object.h"
#include "../core/object/object_db.h"
#include "../scene/main/scene_tree.h"
#include "../scene/main/scene_loader.h"
#include "editor_camera_2d.h"
#include <string>
#include <vector>
#include <deque>
#include <algorithm>

namespace RetroNode {

class RN_API EditorState {
private:
    static EditorState* instance;

    uint64_t selected_instance_id = 0;
    std::unordered_map<std::string, EditorCamera2D> scene_cameras;

    bool is_play_mode = false;
    bool is_paused = false;
    bool game_view_active = false;
    Node* play_mode_root = nullptr;
    bool focus_game_tab_requested = false;
    bool focus_viewport_tab_requested = false;

    // UI Scale / Font Scale
    float ui_scale = 1.0f;

    // Undo / Redo Stacks
    std::deque<std::string> undo_stack;
    std::deque<std::string> redo_stack;
    bool is_undoing_redoing = false;

    // Main Scene & Play Mode Editing State
    std::string main_scene_path = "res://scenes/overworld.json";
    std::string editing_scene_before_play;

    std::string project_dir = "./MyRPG";
    std::string current_scene_path;
    std::vector<std::string> open_scenes;
    std::string selected_node_path_snapshot;

    std::string status_message = "Ready";

public:
    static EditorState* get() {
        if (!instance) {
            instance = new EditorState();
        }
        return instance;
    }

    uint64_t get_selected_instance_id() const { return selected_instance_id; }
    void set_selected_instance_id(uint64_t id) { selected_instance_id = id; }

    Object* get_selected_object() const {
        return ObjectDB::get()->get_object(selected_instance_id);
    }
    Node* get_selected_node() const {
        return dynamic_cast<Node*>(get_selected_object());
    }

    EditorCamera2D& get_camera_for_scene(const std::string& scene_path) {
        std::string path = scene_path.empty() ? current_scene_path : scene_path;
        if (path.empty()) path = "__default__";
        return scene_cameras[path];
    }
    EditorCamera2D& get_camera() { return get_camera_for_scene(current_scene_path); }

    bool get_is_play_mode() const { return is_play_mode; }
    bool get_is_paused() const { return is_paused; }
    bool is_game_view_active() const { return game_view_active; }
    void set_game_view_active(bool active) { game_view_active = active; }
    Node* get_play_mode_root() const { return play_mode_root; }

    Node* get_active_root() const {
        if (is_play_mode && game_view_active && play_mode_root) {
            return play_mode_root;
        }
        return SceneTree::get()->get_root();
    }

    bool pop_focus_game_tab_request() {
        bool req = focus_game_tab_requested;
        focus_game_tab_requested = false;
        return req;
    }
    bool pop_focus_viewport_tab_request() {
        bool req = focus_viewport_tab_requested;
        focus_viewport_tab_requested = false;
        return req;
    }

    void start_play_mode();
    void stop_play_mode();
    void toggle_pause() { is_paused = !is_paused; }

    float get_ui_scale() const { return ui_scale; }
    void set_ui_scale(float s) { ui_scale = std::clamp(s, 0.75f, 2.5f); }

    void push_undo_snapshot();
    void undo();
    void redo();
    bool can_undo() const { return undo_stack.size() > 1; }
    bool can_redo() const { return !redo_stack.empty(); }

    std::string get_main_scene_path() const { return main_scene_path; }
    void set_main_scene_path(const std::string& path) { main_scene_path = path; }

    std::string get_project_dir() const { return project_dir; }
    void set_project_dir(const std::string& dir);

    std::string get_current_scene_path() const { return current_scene_path; }
    void set_current_scene_path(const std::string& path) { current_scene_path = path; }

    const std::vector<std::string>& get_open_scenes() const { return open_scenes; }
    void add_open_scene(const std::string& path) {
        if (path.empty()) return;
        for (const auto& p : open_scenes) {
            if (p == path) return;
        }
        open_scenes.push_back(path);
    }
    void close_open_scene(const std::string& path) {
        open_scenes.erase(std::remove(open_scenes.begin(), open_scenes.end(), path), open_scenes.end());
    }

    std::string get_status_message() const { return status_message; }
    void set_status_message(const std::string& msg) { status_message = msg; }

    void open_scene(const std::string& filepath);
    void save_current_scene();
    void save_current_scene_as(const std::string& filepath);
    void new_scene();
    void reload_instanced_subscenes(const std::string& saved_filepath);
    void reload_instanced_subscenes_in_tree(Node* node, const std::string& saved_filepath);

    std::string get_node_path(const Node* node) const;
    Node* find_node_by_path(Node* root, const std::string& path) const;
};

} // namespace RetroNode

#endif // RETRONODE_EDITOR_STATE_H
