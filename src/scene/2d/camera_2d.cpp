#include "camera_2d.h"
#include "../../servers/visual_server.h"
#include <algorithm>

namespace RetroNode {

Camera2D::Camera2D() {
    name = "Camera2D";
}

void Camera2D::_process(float delta) {
    (void)delta;
    Vector2Fixed global_pos = get_global_position();
    
    int v_w = VisualServer::get()->get_virtual_width();
    int v_h = VisualServer::get()->get_virtual_height();
    Vector2Fixed center_offset = Vector2Fixed::from_floats(v_w / 2.0f, v_h / 2.0f);

    Vector2Fixed cam_pos = global_pos - center_offset;

    if (limit_enabled) {
        float min_x = limit_left;
        float max_x = limit_right - static_cast<float>(v_w);
        float min_y = limit_top;
        float max_y = limit_bottom - static_cast<float>(v_h);

        if (max_x < min_x) max_x = min_x;
        if (max_y < min_y) max_y = min_y;

        float clamped_x = std::clamp(cam_pos.x.to_float(), min_x, max_x);
        float clamped_y = std::clamp(cam_pos.y.to_float(), min_y, max_y);
        cam_pos = Vector2Fixed::from_floats(clamped_x, clamped_y);
    }

    VisualServer::get()->set_camera_offset(cam_pos);
}

} // namespace RetroNode
