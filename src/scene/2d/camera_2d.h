#ifndef RETRONODE_CAMERA_2D_H
#define RETRONODE_CAMERA_2D_H

#include "node_2d.h"

namespace RetroNode {

class RN_API Camera2D : public Node2D {
    RN_CLASS(Camera2D, Node2D)

public:
    bool limit_enabled = true;
    float limit_left = 0.0f;
    float limit_top = 0.0f;
    float limit_right = 256.0f;
    float limit_bottom = 224.0f;

    void set_limit_enabled(bool b) { limit_enabled = b; }
    bool is_limit_enabled() const { return limit_enabled; }

    void set_limit_left(float f) { limit_left = f; }
    float get_limit_left() const { return limit_left; }

    void set_limit_top(float f) { limit_top = f; }
    float get_limit_top() const { return limit_top; }

    void set_limit_right(float f) { limit_right = f; }
    float get_limit_right() const { return limit_right; }

    void set_limit_bottom(float f) { limit_bottom = f; }
    float get_limit_bottom() const { return limit_bottom; }

    Camera2D();
    virtual ~Camera2D() = default;

    virtual void _process(float delta) override;
};

} // namespace RetroNode

#endif // RETRONODE_CAMERA_2D_H
