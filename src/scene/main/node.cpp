#include "node.h"
#include "../gui/canvas_layer.h"
#include "../../core/object/class_db.h"
#include <algorithm>

namespace RetroNode {

Node::Node() : name("Node") {}

Node::~Node() {
    for (Node* child : children) {
        delete child;
    }
    children.clear();
}

void Node::get_property_list(std::vector<PropertyInfo>& out_list) const {
    Object::get_property_list(out_list);
    out_list.push_back({ StringName("name"), VariantType::STRING });
    out_list.push_back({ StringName("script_path"), VariantType::STRING });
}

Variant Node::get(const StringName& p_name) const {
    static const StringName s_name("name");
    static const StringName s_script_path("script_path");
    if (p_name == s_name) return Variant(name);
    if (p_name == s_script_path) return Variant(script_path);
    return Object::get(p_name);
}

bool Node::set(const StringName& p_name, const Variant& p_value) {
    static const StringName s_name("name");
    static const StringName s_script_path("script_path");
    if (p_name == s_name) {
        set_name(p_value.as_string());
        return true;
    }
    if (p_name == s_script_path) {
        set_script_path(p_value.as_string());
        return true;
    }
    return Object::set(p_name, p_value);
}


Node* Node::duplicate() const {
    StringName cls_name = get_class_name();
    Object* new_obj = ClassDB::get()->instantiate(cls_name);
    Node* new_node = dynamic_cast<Node*>(new_obj);
    if (!new_node) {
        if (new_obj) delete new_obj;
        return nullptr;
    }

    // 1. Copy registered ClassDB properties
    std::vector<PropertyInfo> props = ClassDB::get_property_list(cls_name);
    for (const auto& pinfo : props) {
        Variant val = get(pinfo.name);
        if (!val.is_nil()) {
            new_node->set(pinfo.name, val);
        }
    }

    new_node->set_visible(is_visible());
    new_node->set_script_path(script_path);
    new_node->set_scene_instance_path(scene_instance_path);


    // 2. Recursively duplicate child nodes
    for (const Node* child : children) {
        if (child && !child->is_queued_for_deletion) {
            Node* dup_child = child->duplicate();
            if (dup_child) {
                new_node->add_child(dup_child);
            }
        }
    }

    return new_node;
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
    for (size_t i = 0; i < children.size(); ++i) {
        Node* child = children[i];
        if (child && !child->is_free_queued()) {
            child->propagate_ready();
        }
    }
}

void Node::propagate_physics_process(Fixed16 delta) {
    _physics_process(delta);
    for (size_t i = 0; i < children.size(); ++i) {
        Node* child = children[i];
        if (child && !child->is_free_queued()) {
            child->propagate_physics_process(delta);
        }
    }
}

void Node::propagate_process(float delta) {
    _process(delta);
    for (size_t i = 0; i < children.size(); ++i) {
        Node* child = children[i];
        if (child && !child->is_free_queued()) {
            child->propagate_process(delta);
        }
    }
}

} // namespace RetroNode
