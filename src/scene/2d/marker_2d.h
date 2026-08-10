#ifndef RETRONODE_MARKER_2D_H
#define RETRONODE_MARKER_2D_H

#include "node_2d.h"
#include <SDL3/SDL.h>

namespace RetroNode {

class RN_API Marker2D : public Node2D {
    RN_CLASS(Marker2D, Node2D)

public:
    float gizmo_extents = 8.0f;
    SDL_Color gizmo_color = { 255, 64, 64, 255 };

    Marker2D();
    virtual ~Marker2D() = default;

    virtual void _process(float delta) override;
};

} // namespace RetroNode

#endif // RETRONODE_MARKER_2D_H
