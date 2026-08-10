#ifndef RETRONODE_EDITOR_CAMERA_2D_H
#define RETRONODE_EDITOR_CAMERA_2D_H

#include "../core/math/vector2.h"
#include <imgui.h>
#include <algorithm>

namespace RetroNode {

struct EditorCamera2D {
    Vector2Fixed pan = Vector2Fixed::zero();
    float zoom = 1.0f;

    void reset() {
        pan = Vector2Fixed::zero();
        zoom = 1.0f;
    }

    void adjust_zoom(float delta) {
        zoom += delta;
        zoom = std::clamp(zoom, 0.25f, 8.0f);
    }

    ImVec2 world_to_screen(Vector2Fixed world_pos, ImVec2 viewport_origin, ImVec2 padding) const {
        float x = viewport_origin.x + padding.x + (world_pos.x.to_float() - pan.x.to_float()) * zoom;
        float y = viewport_origin.y + padding.y + (world_pos.y.to_float() - pan.y.to_float()) * zoom;
        return ImVec2(x, y);
    }

    Vector2Fixed screen_to_world(ImVec2 screen_pos, ImVec2 viewport_origin, ImVec2 padding) const {
        float wx = pan.x.to_float() + (screen_pos.x - viewport_origin.x - padding.x) / zoom;
        float wy = pan.y.to_float() + (screen_pos.y - viewport_origin.y - padding.y) / zoom;
        return Vector2Fixed::from_floats(wx, wy);
    }
};

} // namespace RetroNode

#endif // RETRONODE_EDITOR_CAMERA_2D_H
