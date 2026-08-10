#include "marker_2d.h"
#include "../../servers/visual_server.h"

namespace RetroNode {

RN_REGISTER_CLASS(Marker2D);

Marker2D::Marker2D() {
    name = "Marker2D";
}

void Marker2D::_process(float delta) {
    Node2D::_process(delta);

    Vector2Fixed global_pos = get_global_position();

    // Render crosshair gizmo lines in debug overlay
    Vector2Fixed h_start = global_pos - Vector2Fixed(Fixed16::from_float(gizmo_extents), Fixed16(0));
    Vector2Fixed h_end   = global_pos + Vector2Fixed(Fixed16::from_float(gizmo_extents), Fixed16(0));
    Vector2Fixed v_start = global_pos - Vector2Fixed(Fixed16(0), Fixed16::from_float(gizmo_extents));
    Vector2Fixed v_end   = global_pos + Vector2Fixed(Fixed16(0), Fixed16::from_float(gizmo_extents));

    VisualServer::get()->draw_line_2d(h_start, h_end, gizmo_color);
    VisualServer::get()->draw_line_2d(v_start, v_end, gizmo_color);
}

} // namespace RetroNode
