#include "nine_patch_rect.h"
#include "../../servers/visual_server.h"
#include "../../servers/texture_server.h"

namespace RetroNode {

NinePatchRect::NinePatchRect() {
    name = "NinePatchRect";
    size = Vector2Fixed::from_floats(160.0f, 48.0f);
    z_index = 90;
}

void NinePatchRect::set_texture_path(const std::string& path) {
    texture_path = path;
    if (!texture_path.empty()) {
        texture_id = TextureServer::get()->load_texture(texture_path);
    }
}

void NinePatchRect::_process(float delta) {
    (void)delta;

    Vector2Fixed global_pos = get_global_control_position();
    Vector2Fixed global_prev_pos = get_global_control_previous_position();
    Vector2Fixed cam_offset = VisualServer::get()->get_camera_offset();

    Vector2Fixed ui_draw_pos = global_pos + cam_offset;
    Vector2Fixed ui_prev_draw_pos = global_prev_pos + cam_offset;

    Vector2Fixed tex_size = TextureServer::get()->get_texture_size(texture_id);
    if (tex_size.x <= Fixed16(0) || tex_size.y <= Fixed16(0)) {
        // Fallback procedural retro dialog box if texture not assigned
        VisualServer::get()->submit_draw_sprite(
            ui_draw_pos,
            ui_prev_draw_pos,
            size,
            Rect2Fixed::from_floats(0.0f, 0.0f, 16.0f, 16.0f),
            0,
            z_index,
            { 20, 20, 35, 230 }
        );
        return;
    }

    float m = static_cast<float>(patch_margin);
    float tw = tex_size.x.to_float();
    float th = tex_size.y.to_float();

    float dw = size.x.to_float();
    float dh = size.y.to_float();

    float src_x[3] = { 0.0f, m, tw - m };
    float src_w[3] = { m, tw - 2.0f * m, m };

    float src_y[3] = { 0.0f, m, th - m };
    float src_h[3] = { m, th - 2.0f * m, m };

    float dst_x[3] = { 0.0f, m, dw - m };
    float dst_w[3] = { m, dw - 2.0f * m, m };

    float dst_y[3] = { 0.0f, m, dh - m };
    float dst_h[3] = { m, dh - 2.0f * m, m };

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            Vector2Fixed quad_pos(
                ui_draw_pos.x + Fixed16::from_float(dst_x[c]),
                ui_draw_pos.y + Fixed16::from_float(dst_y[r])
            );

            Vector2Fixed quad_prev_pos(
                ui_prev_draw_pos.x + Fixed16::from_float(dst_x[c]),
                ui_prev_draw_pos.y + Fixed16::from_float(dst_y[r])
            );

            Vector2Fixed quad_sz = Vector2Fixed::from_floats(dst_w[c], dst_h[r]);
            Rect2Fixed src_rect = Rect2Fixed::from_floats(src_x[c], src_y[r], src_w[c], src_h[r]);

            VisualServer::get()->submit_draw_sprite(
                quad_pos,
                quad_prev_pos,
                quad_sz,
                src_rect,
                texture_id,
                z_index,
                modulate
            );
        }
    }
}

} // namespace RetroNode
