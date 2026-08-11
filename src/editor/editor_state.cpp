#include "editor_state.h"
#include "../scene/2d/node_2d.h"
#include "../servers/physics_server.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <sstream>

using json = nlohmann::json;

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

void EditorState::set_project_dir(const std::string& dir) {
    project_dir = dir;
    std::string pfile = project_dir + "/project.rnode";
    std::ifstream file(pfile);
    if (file.is_open()) {
        try {
            json j;
            file >> j;
            if (j.contains("main_scene") && j["main_scene"].is_string()) {
                main_scene_path = j["main_scene"].get<std::string>();
                std::cout << "[Editor] Parsed main_scene from project.rnode: " << main_scene_path << std::endl;
            }
        } catch (...) {}
    }
}

void EditorState::push_undo_snapshot() {
    if (is_play_mode || is_undoing_redoing) return;
    Node* root = SceneTree::get()->get_root();
    if (!root) return;

    std::string snapshot = SceneLoader::serialize_node_to_json_string(root);
    if (undo_stack.empty() || undo_stack.back() != snapshot) {
        undo_stack.push_back(snapshot);
        if (undo_stack.size() > 50) {
            undo_stack.pop_front();
        }
        redo_stack.clear();
    }
}

void EditorState::undo() {
    if (undo_stack.size() <= 1) return;
    is_undoing_redoing = true;

    redo_stack.push_back(undo_stack.back());
    undo_stack.pop_back();

    std::string prev_snapshot = undo_stack.back();
    PhysicsServer2D::get()->clear();
    Node* restored = SceneLoader::load_scene_from_json_string(prev_snapshot);
    if (restored) {
        SceneTree::get()->set_root(restored);
        status_message = "Undo performed";
    }

    is_undoing_redoing = false;
}

void EditorState::redo() {
    if (redo_stack.empty()) return;
    is_undoing_redoing = true;

    std::string next_snapshot = redo_stack.back();
    redo_stack.pop_back();
    undo_stack.push_back(next_snapshot);

    PhysicsServer2D::get()->clear();
    Node* restored = SceneLoader::load_scene_from_json_string(next_snapshot);
    if (restored) {
        SceneTree::get()->set_root(restored);
        status_message = "Redo performed";
    }

    is_undoing_redoing = false;
}

void EditorState::start_play_mode() {
    if (is_play_mode) return;

    editing_scene_before_play = current_scene_path;

    if (play_mode_root) {
        delete play_mode_root;
        play_mode_root = nullptr;
    }

    play_mode_root = SceneLoader::load_scene_from_file(main_scene_path);
    if (!play_mode_root && !current_scene_path.empty()) {
        play_mode_root = SceneLoader::load_scene_from_file(current_scene_path);
    }

    if (play_mode_root) {
        play_mode_root->propagate_ready();
    }

    is_play_mode = true;
    is_paused = false;
    focus_game_tab_requested = true;
    status_message = "Play mode active (Running: " + main_scene_path + ")";
    std::cout << "[Editor] Started sandboxed Play mode on main scene: " << main_scene_path << std::endl;
}

void EditorState::stop_play_mode() {
    if (!is_play_mode) return;

    is_play_mode = false;
    is_paused = false;
    focus_viewport_tab_requested = true;

    if (play_mode_root) {
        delete play_mode_root;
        play_mode_root = nullptr;
    }

    status_message = "Play mode stopped";
    std::cout << "[Editor] Stopped Play mode." << std::endl;
}

void EditorState::open_scene(const std::string& filepath) {
    if (filepath.empty()) return;

    // Save initial state for undo stack before opening new scene
    if (current_scene_path != filepath) {
        set_selected_instance_id(0);
        PhysicsServer2D::get()->clear();

        Node* new_root = SceneLoader::load_scene_from_file(filepath);
        if (new_root) {
            SceneTree::get()->set_root(new_root);
            current_scene_path = filepath;
            add_open_scene(filepath);
            undo_stack.clear();
            redo_stack.clear();
            push_undo_snapshot();
            status_message = "Opened scene: " + filepath;
        } else {
            status_message = "Failed to open scene: " + filepath;
        }
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
        reload_instanced_subscenes(current_scene_path);
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
        reload_instanced_subscenes(filepath);
    } else {
        status_message = "Error saving scene to " + filepath;
    }
}

void EditorState::reload_instanced_subscenes_in_tree(Node* node, const std::string& saved_filepath) {
    if (!node) return;
    std::vector<Node*> children = node->get_children();
    for (size_t i = 0; i < children.size(); ++i) {
        Node* child = children[i];
        if (child && child->is_instanced_subscene()) {
            std::string inst_path = child->get_scene_instance_path();
            std::string res_inst = inst_path;
            if (res_inst.rfind("res://", 0) == 0) res_inst = res_inst.substr(6);
            std::string res_saved = saved_filepath;
            if (res_saved.rfind("res://", 0) == 0) res_saved = res_saved.substr(6);

            size_t slash1 = res_inst.find_last_of("/\\");
            std::string fname1 = (slash1 != std::string::npos) ? res_inst.substr(slash1 + 1) : res_inst;
            size_t slash2 = res_saved.find_last_of("/\\");
            std::string fname2 = (slash2 != std::string::npos) ? res_saved.substr(slash2 + 1) : res_saved;

            if (inst_path == saved_filepath || res_inst == res_saved || fname1 == fname2) {
                std::string cname = child->get_name();
                Node2D* n2d = dynamic_cast<Node2D*>(child);
                Vector2Fixed pos = n2d ? n2d->get_position() : Vector2Fixed::zero();
                Fixed16 rot = n2d ? n2d->get_rotation() : Fixed16(0);
                Vector2Fixed scl = n2d ? n2d->get_scale() : Vector2Fixed::one();

                Node* new_inst = SceneLoader::load_scene_from_file(saved_filepath);
                if (new_inst) {
                    new_inst->set_scene_instance_path(inst_path);
                    new_inst->set_name(cname);
                    Node2D* new_n2d = dynamic_cast<Node2D*>(new_inst);
                    if (new_n2d) {
                        new_n2d->set_position(pos);
                        new_n2d->set_rotation(rot);
                        new_n2d->set_scale(scl);
                    }
                    node->remove_child(child);
                    delete child;
                    node->add_child(new_inst);
                    std::cout << "[EditorState] Live reloaded instanced subscene: " << cname << " (" << inst_path << ")" << std::endl;
                    continue;
                }
            }
        }
        reload_instanced_subscenes_in_tree(child, saved_filepath);
    }
}

void EditorState::reload_instanced_subscenes(const std::string& saved_filepath) {
    if (saved_filepath.empty()) return;
    reload_instanced_subscenes_in_tree(SceneTree::get()->get_root(), saved_filepath);
    if (play_mode_root) {
        reload_instanced_subscenes_in_tree(play_mode_root, saved_filepath);
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
