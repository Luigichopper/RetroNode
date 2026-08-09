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

    Camera2D();
    virtual ~Camera2D() = default;

    virtual void _process(float delta) override;
};

} // namespace RetroNode

#endif // RETRONODE_CAMERA_2D_H
