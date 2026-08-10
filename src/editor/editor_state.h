#ifndef RETRONODE_EDITOR_STATE_H
#define RETRONODE_EDITOR_STATE_H

#include "../core/object/object.h"
#include "../core/object/object_db.h"
#include "../scene/main/scene_tree.h"
#include "../scene/main/scene_loader.h"
#include "editor_camera_2d.h"
#include <string>

namespace RetroNode {

class RN_API EditorState {
private:
    static EditorState* instance;

    uint64_t selected_instance_id = 0;
    EditorCamera2D camera;

    bool is_play_mode = false;
    bool is_paused = false;

    std::string project_dir = "./MyRPG";
    std::string current_scene_path;
    std::vector<std::string> open_scenes;
    std::string play_mode_snapshot_json;
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

    EditorCamera2D& get_camera() { return camera; }

    bool get_is_play_mode() const { return is_play_mode; }
    bool get_is_paused() const { return is_paused; }

    void start_play_mode();
    void stop_play_mode();
    void toggle_pause() { is_paused = !is_paused; }

    std::string get_project_dir() const { return project_dir; }
    void set_project_dir(const std::string& dir) { project_dir = dir; }

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

    std::string get_node_path(const Node* node) const;
    Node* find_node_by_path(Node* root, const std::string& path) const;
};

} // namespace RetroNode

#endif // RETRONODE_EDITOR_STATE_H
