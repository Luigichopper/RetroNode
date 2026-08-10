#include "scene_tree.h"
#include <vector>

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
    Node* old_root = root_node;
    root_node = nullptr;
    if (old_root) {
        delete old_root;
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

void SceneTree::cleanup_queued_nodes(Node* node) {
    if (!node) return;

    std::vector<Node*> to_remove;
    for (Node* child : node->get_children()) {
        if (child->is_free_queued()) {
            to_remove.push_back(child);
        } else {
            cleanup_queued_nodes(child);
        }
    }

    for (Node* child : to_remove) {
        node->remove_child(child);
        delete child;
    }
}

void SceneTree::process(float delta) {
    if (root_node) {
        root_node->propagate_process(delta);
        cleanup_queued_nodes(root_node);
    }
}

} // namespace RetroNode
