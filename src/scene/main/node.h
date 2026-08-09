#ifndef RETRONODE_NODE_H
#define RETRONODE_NODE_H

#include "../../core/object/object.h"
#include "../../core/object/class_db.h"
#include "../../core/math/fixed16.h"
#include <vector>
#include <string>
#include <memory>

namespace RetroNode {

class SceneTree;

class RN_API Node : public Object {
    RN_CLASS(Node, Object)

protected:
    std::string name;
    Node* parent = nullptr;
    std::vector<Node*> children;
    bool is_queued_for_deletion = false;

public:
    Node();
    virtual ~Node();

    void set_name(const std::string& p_name) { name = p_name; }
    const std::string& get_name() const { return name; }

    Node* get_parent() const { return parent; }
    const std::vector<Node*>& get_children() const { return children; }

    void add_child(Node* child);
    void remove_child(Node* child);

    template<typename T>
    T* get_node(const std::string& path) const {
        for (Node* child : children) {
            if (child->get_name() == path) {
                T* typed = dynamic_cast<T*>(child);
                if (typed) return typed;
            }
        }
        return nullptr;
    }

    void queue_free() { is_queued_for_deletion = true; }
    bool is_free_queued() const { return is_queued_for_deletion; }

    virtual void _ready() {}
    virtual void _process(float delta) { (void)delta; }
    virtual void _physics_process(Fixed16 delta) { (void)delta; }

    void propagate_ready();
    void propagate_physics_process(Fixed16 delta);
    void propagate_process(float delta);
};

} // namespace RetroNode

#endif // RETRONODE_NODE_H
