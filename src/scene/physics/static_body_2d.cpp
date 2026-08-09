#include "static_body_2d.h"
#include "../../core/object/class_db.h"
#include "../../servers/physics_server.h"

namespace RetroNode {

RN_REGISTER_CLASS(StaticBody2D)

StaticBody2D::StaticBody2D() {
    name = "StaticBody2D";
}

void StaticBody2D::_ready() {
    Node2D::_ready();
    update_shapes();
    register_shapes();
}

void StaticBody2D::update_shapes() {
    collision_shapes.clear();
    for (Node* child : get_children()) {
        if (CollisionShape2D* shape = dynamic_cast<CollisionShape2D*>(child)) {
            collision_shapes.push_back(shape);
        }
    }
}

void StaticBody2D::register_shapes() {
    uint64_t base_id = get_instance_id();
    size_t idx = 0;
    for (CollisionShape2D* shape : collision_shapes) {
        if (!shape) continue;
        Rect2Fixed box = shape->get_global_box();
        PhysicsServer2D::get()->add_static_box(base_id + idx, box);
        idx++;
    }
}

} // namespace RetroNode
