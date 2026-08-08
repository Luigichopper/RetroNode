#ifndef RETRONODE_CAMERA_2D_H
#define RETRONODE_CAMERA_2D_H

#include "node_2d.h"

namespace RetroNode {

class Camera2D : public Node2D {
    RN_CLASS(Camera2D, Node2D)

public:
    Camera2D();
    virtual ~Camera2D() = default;

    virtual void _process(float delta) override;
};

} // namespace RetroNode

#endif // RETRONODE_CAMERA_2D_H
