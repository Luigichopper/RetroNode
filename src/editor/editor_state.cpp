#include "editor_state.h"
#include "../scene/2d/node_2d.h"
#include "../servers/physics_server.h"
#include <iostream>
#include <sstream>

namespace RetroNode {

EditorState* EditorState::instance = nullptr;

std::string EditorState::get_node_path(const Node* node) const {
    if (!node) return "";
    std::string path = node->get_name();
    const Node* p = node->get_parent();
    while (p) {
        path = p->get_name() + "/" + path;
        p = p->get_parent();
    }
    return "/" + path;
}

Node* EditorState::find_node_by_path(Node* root, const std::string& path) const {
    if (!root || path.empty()) return nullptr;
    if (get_node_path(root) == path) return root;

    const auto& children = root->get_children();
    for (size_t i = 0; i < children.size(); ++i) {
        Node* found = find_node_by_path(children[i], path);
        if (found) return found;
    }
    return nullptr;
}

void EditorState::start_play_mode() {
    if (is_play_mode) return;

    Node* root = SceneTree::get()->get_root();
    if (root) {
        play_mode_snapshot_json = SceneLoader::serialize_node_to_json_string(root);
        selected_node_path_snapshot = get_node_path(get_selected_node());
    }

    is_play_mode = true;
    is_paused = false;
    status_message = "Play mode active";
    std::cout << "[Editor] Started sandboxed Play mode." << std::endl;
}

void EditorState::stop_play_mode() {
    if (!is_play_mode) return;

    is_play_mode = false;
    is_paused = false;
    selected_instance_id = 0;

    // Clear stale physics body references before node destruction
    PhysicsServer2D::get()->clear();

    if (!play_mode_snapshot_json.empty()) {
        Node* restored_root = SceneLoader::load_scene_from_json_string(play_mode_snapshot_json);
        SceneTree::get()->set_root(restored_root);
        if (restored_root && !selected_node_path_snapshot.empty()) {
            Node* target = find_node_by_path(restored_root, selected_node_path_snapshot);
            if (target) {
                set_selected_instance_id(target->get_instance_id());
            }
        }
    }

    status_message = "Play mode stopped - restored scene state";
    std::cout << "[Editor] Stopped Play mode and restored scene snapshot." << std::endl;
}

void EditorState::open_scene(const std::string& filepath) {
    if (filepath.empty()) return;
    set_selected_instance_id(0);
    PhysicsServer2D::get()->clear();

    Node* new_root = SceneLoader::load_scene_from_file(filepath);
    if (new_root) {
        SceneTree::get()->set_root(new_root);
        current_scene_path = filepath;
        add_open_scene(filepath);
        status_message = "Opened scene: " + filepath;
    } else {
        status_message = "Failed to open scene: " + filepath;
    }
}

void EditorState::save_current_scene() {
    Node* root = SceneTree::get()->get_root();
    if (!root) {
        status_message = "Cannot save: empty scene tree";
        return;
    }
    if (current_scene_path.empty()) {
        save_current_scene_as(project_dir + "/scenes/untitled.json");
        return;
    }
    if (SceneLoader::save_scene_to_file(root, current_scene_path)) {
        status_message = "Saved scene: " + current_scene_path;
    } else {
        status_message = "Error saving scene to " + current_scene_path;
    }
}

void EditorState::save_current_scene_as(const std::string& filepath) {
    Node* root = SceneTree::get()->get_root();
    if (!root) return;
    if (SceneLoader::save_scene_to_file(root, filepath)) {
        current_scene_path = filepath;
        status_message = "Saved scene as: " + filepath;
    } else {
        status_message = "Error saving scene to " + filepath;
    }
}

void EditorState::new_scene() {
    set_selected_instance_id(0);
    PhysicsServer2D::get()->clear();

    Node2D* new_root = new Node2D();
    new_root->set_name("Root2D");
    SceneTree::get()->set_root(new_root);
    current_scene_path = "";
    set_selected_instance_id(new_root->get_instance_id());
    status_message = "Created new scene";
}

} // namespace RetroNode
