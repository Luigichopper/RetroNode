#include "scene_loader.h"
#include "../../core/object/class_db.h"
#include "../2d/node_2d.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

namespace RetroNode {

static Node* parse_node_internal(const json& j) {
    std::string type_name = j.value("type", "Node");
    std::string script_name = j.value("script", "");

    std::string create_type = script_name.empty() ? type_name : script_name;

    Object* obj = ClassDB::get()->instantiate(StringName(create_type));
    if (!obj && !script_name.empty()) {
        std::cout << "[SceneLoader] ClassDB fallback to base type '" << type_name << "' for script '" << script_name << "'" << std::endl;
        obj = ClassDB::get()->instantiate(StringName(type_name));
    }

    Node* node = dynamic_cast<Node*>(obj);
    if (!node) {
        std::cerr << "[SceneLoader] Failed to create node of type '" << create_type << "', creating default Node" << std::endl;
        node = new Node();
    }

    if (j.contains("name")) {
        node->set_name(j["name"]);
    }

    // Apply properties if present
    if (j.contains("properties")) {
        const auto& props = j["properties"];
        Node2D* node2d = dynamic_cast<Node2D*>(node);
        if (node2d && props.contains("position")) {
            float px = props["position"].value("x", 0.0f);
            float py = props["position"].value("y", 0.0f);
            node2d->position = Vector2Fixed::from_floats(px, py);
        }
    }

    // Parse children recursively
    if (j.contains("children") && j["children"].is_array()) {
        for (const auto& child_j : j["children"]) {
            Node* child_node = parse_node_internal(child_j);
            if (child_node) {
                node->add_child(child_node);
            }
        }
    }

    return node;
}

Node* SceneLoader::load_scene_from_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[SceneLoader] Failed to open scene file: " << filepath << std::endl;
        return nullptr;
    }

    try {
        json j;
        file >> j;
        return parse_node_internal(j);
    } catch (const std::exception& e) {
        std::cerr << "[SceneLoader] JSON Parse Error: " << e.what() << std::endl;
        return nullptr;
    }
}

} // namespace RetroNode
