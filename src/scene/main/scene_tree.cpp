#include "scene_tree.h"

namespace RetroNode {

SceneTree* SceneTree::instance = nullptr;

SceneTree::SceneTree() {}

SceneTree::~SceneTree() {
    if (root_node) {
        delete root_node;
        root_node = nullptr;
    }
}

void SceneTree::set_root(Node* node) {
    if (root_node) {
        delete root_node;
    }
    root_node = node;
    if (root_node) {
        root_node->propagate_ready();
    }
}

void SceneTree::physics_process(Fixed16 delta) {
    if (root_node) {
        root_node->propagate_physics_process(delta);
    }
}

void SceneTree::process(float delta) {
    if (root_node) {
        root_node->propagate_process(delta);
    }
}

} // namespace RetroNode
