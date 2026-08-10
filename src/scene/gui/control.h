#ifndef RETRONODE_CONTROL_H
#define RETRONODE_CONTROL_H

#include "../main/node.h"
#include "../../core/math/vector2.h"
#include "../../core/math/rect2.h"

namespace RetroNode {

class RN_API Control : public Node {
    RN_CLASS(Control, Node)

public:
    Vector2Fixed position;
    Vector2Fixed previous_position;
    Vector2Fixed size;
    int z_index = 100;

    Control();
    virtual ~Control() = default;

    virtual void get_property_list(std::vector<PropertyInfo>& out_list) const override;
    virtual Variant get(const StringName& p_name) const override;
    virtual bool set(const StringName& p_name, const Variant& p_value) override;

    Vector2Fixed get_global_control_position() const;
    Vector2Fixed get_global_control_previous_position() const;

    void set_position(const Vector2Fixed& pos) {
        position = pos;
        previous_position = pos;
    }
    Vector2Fixed get_position() const { return position; }

    void set_size(const Vector2Fixed& sz) { size = sz; }
    Vector2Fixed get_size() const { return size; }

    virtual void _physics_process(Fixed16 delta) override {
        previous_position = position;
        Node::_physics_process(delta);
    }
};

} // namespace RetroNode

#endif // RETRONODE_CONTROL_H
