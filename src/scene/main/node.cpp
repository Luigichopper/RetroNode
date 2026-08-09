#include "node.h"
#include "../gui/canvas_layer.h"
#include <algorithm>

namespace RetroNode {

Node::Node() : name("Node") {}

Node::~Node() {
    for (Node* child : children) {
        delete child;
    }
    children.clear();
}

CanvasLayer* Node::get_canvas_layer() const {
    const Node* curr = this;
    while (curr) {
        const CanvasLayer* cl = dynamic_cast<const CanvasLayer*>(curr);
        if (cl) return const_cast<CanvasLayer*>(cl);
        curr = curr->get_parent();
    }
    return nullptr;
}

void Node::add_child(Node* child) {
    if (!child) return;
    if (child->parent) {
        child->parent->remove_child(child);
    }
    child->parent = this;
    children.push_back(child);
}

void Node::remove_child(Node* child) {
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end()) {
        (*it)->parent = nullptr;
        children.erase(it);
    }
}

void Node::propagate_ready() {
    _ready();
    auto children_copy = children;
    for (Node* child : children_copy) {
        if (child && !child->is_free_queued()) {
            child->propagate_ready();
        }
    }
}

void Node::propagate_physics_process(Fixed16 delta) {
    _physics_process(delta);
    auto children_copy = children;
    for (Node* child : children_copy) {
        if (child && !child->is_free_queued()) {
            child->propagate_physics_process(delta);
        }
    }
}

void Node::propagate_process(float delta) {
    _process(delta);
    auto children_copy = children;
    for (Node* child : children_copy) {
        if (child && !child->is_free_queued()) {
            child->propagate_process(delta);
        }
    }
}

} // namespace RetroNode
