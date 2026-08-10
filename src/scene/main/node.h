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
    std::string scene_instance_path;
    std::string script_path;
    Node* parent = nullptr;
    std::vector<Node*> children;
    bool is_queued_for_deletion = false;
    bool visible = true;

public:
    Node();
    virtual ~Node();

    Node* duplicate() const;

    using Object::get_property_list;
    virtual void get_property_list(std::vector<PropertyInfo>& out_list) const override;
    virtual Variant get(const StringName& p_name) const override;
    virtual bool set(const StringName& p_name, const Variant& p_value) override;

    void set_name(const std::string& p_name) { name = p_name; }
    const std::string& get_name() const { return name; }

    void set_scene_instance_path(const std::string& path) { scene_instance_path = path; }
    const std::string& get_scene_instance_path() const { return scene_instance_path; }
    bool is_instanced_subscene() const { return !scene_instance_path.empty(); }

    void set_script_path(const std::string& path) { script_path = path; }
    const std::string& get_script_path() const { return script_path; }
    bool has_script() const {
        if (!script_path.empty()) return true;
        std::string cname = get_class_name().as_string();
        return (cname != "Node" && cname != "Node2D" && cname != "Marker2D" &&
                cname != "Sprite2D" && cname != "AnimatedSprite2D" &&
                cname != "TileMapLayer" && cname != "CharacterBody2D" &&
                cname != "Camera2D" && cname != "Control" && cname != "CanvasLayer" &&
                cname != "Label" && cname != "NinePatchRect" && cname != "DebugOverlay" &&
                cname != "AudioStreamPlayer" && cname != "AnimationPlayer" &&
                cname != "Timer" && cname != "CPUParticles2D");
    }

    Node* get_parent() const { return parent; }
    const std::vector<Node*>& get_children() const { return children; }

    void set_visible(bool p_visible) { visible = p_visible; }
    bool is_visible() const { return visible; }
    bool is_visible_in_tree() const {
        if (!visible) return false;
        if (parent) return parent->is_visible_in_tree();
        return true;
    }

    class CanvasLayer* get_canvas_layer() const;

    void add_child(Node* child);
    void remove_child(Node* child);

    template<typename T>
    T* get_node(const std::string& path) const {
        if (path.empty()) return nullptr;
        size_t slash = path.find('/');
        if (slash == std::string::npos) {
            for (Node* child : children) {
                if (child->get_name() == path) {
                    return dynamic_cast<T*>(child);
                }
            }
            return nullptr;
        }
        std::string first = path.substr(0, slash);
        std::string rest = path.substr(slash + 1);
        for (Node* child : children) {
            if (child->get_name() == first) {
                return child->get_node<T>(rest);
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
