#include "camera_2d.h"
#include "../../servers/visual_server.h"

namespace RetroNode {

Camera2D::Camera2D() {
    name = "Camera2D";
}

void Camera2D::_process(float delta) {
    (void)delta;
    Vector2Fixed global_pos = get_global_position();
    
    // Center camera on target retro framebuffer
    int v_w = VisualServer::get()->get_virtual_width();
    int v_h = VisualServer::get()->get_virtual_height();
    Vector2Fixed center_offset = Vector2Fixed::from_floats(v_w / 2.0f, v_h / 2.0f);

    Vector2Fixed cam_pos = global_pos - center_offset;
    VisualServer::get()->set_camera_offset(cam_pos);
}

} // namespace RetroNode
