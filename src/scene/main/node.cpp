#include "node.h"
#include <algorithm>

namespace RetroNode {

Node::Node() : name("Node") {}

Node::~Node() {
    for (Node* child : children) {
        delete child;
    }
    children.clear();
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
    for (Node* child : children) {
        child->propagate_ready();
    }
}

void Node::propagate_physics_process(Fixed16 delta) {
    _physics_process(delta);
    for (Node* child : children) {
        child->propagate_physics_process(delta);
    }
}

void Node::propagate_process(float delta) {
    _process(delta);
    for (Node* child : children) {
        child->propagate_process(delta);
    }
}

} // namespace RetroNode
