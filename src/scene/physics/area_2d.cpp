#include "area_2d.h"
#include "../../core/object/class_db.h"
#include "../../servers/physics_server.h"
#include <algorithm>

namespace RetroNode {

RN_REGISTER_CLASS(Area2D)

Area2D::Area2D() {
    name = "Area2D";
}

void Area2D::_ready() {
    Node2D::_ready();
    update_shapes();
}

void Area2D::update_shapes() {
    collision_shapes.clear();
    for (Node* child : get_children()) {
        if (CollisionShape2D* shape = dynamic_cast<CollisionShape2D*>(child)) {
            collision_shapes.push_back(shape);
        }
    }
}

void Area2D::notify_body_entered(Node2D* p_node) {
    if (!p_node) return;
    if (std::find(overlapping_nodes.begin(), overlapping_nodes.end(), p_node) == overlapping_nodes.end()) {
        overlapping_nodes.push_back(p_node);
        if (on_body_entered) {
            on_body_entered(p_node);
        }
    }
}

void Area2D::notify_body_exited(Node2D* p_node) {
    if (!p_node) return;
    auto it = std::find(overlapping_nodes.begin(), overlapping_nodes.end(), p_node);
    if (it != overlapping_nodes.end()) {
        overlapping_nodes.erase(it);
        if (on_body_exited) {
            on_body_exited(p_node);
        }
    }
}

void Area2D::_physics_process(Fixed16 delta) {
    Node2D::_physics_process(delta);
    
    if (collision_shapes.empty()) {
        update_shapes();
    }

    if (collision_shapes.empty()) {
        return;
    }

    // Check overlap using PhysicsServer2D for active registered nodes
    std::vector<Node2D*> currently_overlapping;
    for (CollisionShape2D* shape : collision_shapes) {
        if (!shape) continue;
        Rect2Fixed area_box = shape->get_global_box();
        std::vector<Node2D*> detected = PhysicsServer2D::get()->get_overlapping_bodies_for_box(area_box, this);
        for (Node2D* body : detected) {
            if (std::find(currently_overlapping.begin(), currently_overlapping.end(), body) == currently_overlapping.end()) {
                currently_overlapping.push_back(body);
            }
        }
    }

    // Process exited nodes
    std::vector<Node2D*> to_remove;
    for (Node2D* old_node : overlapping_nodes) {
        if (std::find(currently_overlapping.begin(), currently_overlapping.end(), old_node) == currently_overlapping.end()) {
            to_remove.push_back(old_node);
        }
    }
    for (Node2D* old_node : to_remove) {
        notify_body_exited(old_node);
    }

    // Process entered nodes
    for (Node2D* new_node : currently_overlapping) {
        notify_body_entered(new_node);
    }
}

} // namespace RetroNode
