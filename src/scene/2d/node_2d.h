#ifndef RETRONODE_NODE_2D_H
#define RETRONODE_NODE_2D_H

#include "../main/node.h"
#include "../../core/math/vector2.h"
#include "../../core/math/rect2.h"

namespace RetroNode {

class RN_API Node2D : public Node {
    RN_CLASS(Node2D, Node)

public:
    Vector2Fixed position;
    Vector2Fixed previous_position;
    Fixed16 rotation;
    Vector2Fixed scale;

    Node2D();
    virtual ~Node2D() = default;

    virtual void get_property_list(std::vector<PropertyInfo>& out_list) const override;
    virtual Variant get(const StringName& p_name) const override;
    virtual bool set(const StringName& p_name, const Variant& p_value) override;

    Vector2Fixed get_global_position() const;
    Vector2Fixed get_global_previous_position() const;
    Fixed16 get_global_rotation() const;
    Vector2Fixed get_global_scale() const;

    void set_position(const Vector2Fixed& pos) { 
        position = pos;
    }
    void reset_physics_interpolation() {
        previous_position = position;
    }
    Vector2Fixed get_position() const { return position; }

    void set_rotation(Fixed16 p_rot) { rotation = p_rot; }
    Fixed16 get_rotation() const { return rotation; }

    void set_scale(const Vector2Fixed& p_scale) { scale = p_scale; }
    Vector2Fixed get_scale() const { return scale; }

    virtual void _physics_process(Fixed16 delta) override {
        previous_position = position;
        Node::_physics_process(delta);
    }
};

} // namespace RetroNode

#endif // RETRONODE_NODE_2D_H
