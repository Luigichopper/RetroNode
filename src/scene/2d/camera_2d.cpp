#include "camera_2d.h"
#include "../../servers/visual_server.h"
#include "../../core/object/class_db.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace RetroNode {

Camera2D* Camera2D::current_camera = nullptr;

Camera2D::Camera2D() {
    name = "Camera2D";
    if (!current_camera) {
        current_camera = this;
    }
}

Camera2D::~Camera2D() {
    if (current_camera == this) {
        current_camera = nullptr;
    }
}

void Camera2D::get_property_list(std::vector<PropertyInfo>& out_list) const {
    Node2D::get_property_list(out_list);
    out_list.push_back({ StringName("current"), VariantType::BOOL });
    out_list.push_back({ StringName("mode"), VariantType::INT });
    out_list.push_back({ StringName("smoothing_enabled"), VariantType::BOOL });
    out_list.push_back({ StringName("smoothing_speed"), VariantType::FLOAT16 });
    out_list.push_back({ StringName("drag_margin_enabled"), VariantType::BOOL });
    out_list.push_back({ StringName("drag_margin_left"), VariantType::FLOAT16 });
    out_list.push_back({ StringName("drag_margin_top"), VariantType::FLOAT16 });
    out_list.push_back({ StringName("drag_margin_right"), VariantType::FLOAT16 });
    out_list.push_back({ StringName("drag_margin_bottom"), VariantType::FLOAT16 });
    out_list.push_back({ StringName("room_width"), VariantType::INT });
    out_list.push_back({ StringName("room_height"), VariantType::INT });
    out_list.push_back({ StringName("limit_enabled"), VariantType::BOOL });
    out_list.push_back({ StringName("limit_left"), VariantType::FLOAT16 });
    out_list.push_back({ StringName("limit_top"), VariantType::FLOAT16 });
    out_list.push_back({ StringName("limit_right"), VariantType::FLOAT16 });
    out_list.push_back({ StringName("limit_bottom"), VariantType::FLOAT16 });
}

Variant Camera2D::get(const StringName& p_name) const {
    static const StringName s_current("current");
    static const StringName s_mode("mode");
    static const StringName s_smoothing_enabled("smoothing_enabled");
    static const StringName s_smoothing_speed("smoothing_speed");
    static const StringName s_drag_margin_enabled("drag_margin_enabled");
    static const StringName s_drag_margin_left("drag_margin_left");
    static const StringName s_drag_margin_top("drag_margin_top");
    static const StringName s_drag_margin_right("drag_margin_right");
    static const StringName s_drag_margin_bottom("drag_margin_bottom");
    static const StringName s_room_width("room_width");
    static const StringName s_room_height("room_height");
    static const StringName s_limit_enabled("limit_enabled");
    static const StringName s_limit_left("limit_left");
    static const StringName s_limit_top("limit_top");
    static const StringName s_limit_right("limit_right");
    static const StringName s_limit_bottom("limit_bottom");

    if (p_name == s_current) return Variant(current);
    if (p_name == s_mode) return Variant((int64_t)mode);
    if (p_name == s_smoothing_enabled) return Variant(smoothing_enabled);
    if (p_name == s_smoothing_speed) return Variant(Fixed16::from_float(smoothing_speed));
    if (p_name == s_drag_margin_enabled) return Variant(drag_margin_enabled);
    if (p_name == s_drag_margin_left) return Variant(Fixed16::from_float(drag_margin_left));
    if (p_name == s_drag_margin_top) return Variant(Fixed16::from_float(drag_margin_top));
    if (p_name == s_drag_margin_right) return Variant(Fixed16::from_float(drag_margin_right));
    if (p_name == s_drag_margin_bottom) return Variant(Fixed16::from_float(drag_margin_bottom));
    if (p_name == s_room_width) return Variant((int64_t)room_width);
    if (p_name == s_room_height) return Variant((int64_t)room_height);
    if (p_name == s_limit_enabled) return Variant(limit_enabled);
    if (p_name == s_limit_left) return Variant(Fixed16::from_float(limit_left));
    if (p_name == s_limit_top) return Variant(Fixed16::from_float(limit_top));
    if (p_name == s_limit_right) return Variant(Fixed16::from_float(limit_right));
    if (p_name == s_limit_bottom) return Variant(Fixed16::from_float(limit_bottom));

    return Node2D::get(p_name);
}

bool Camera2D::set(const StringName& p_name, const Variant& p_value) {
    static const StringName s_current("current");
    static const StringName s_mode("mode");
    static const StringName s_smoothing_enabled("smoothing_enabled");
    static const StringName s_smoothing_speed("smoothing_speed");
    static const StringName s_drag_margin_enabled("drag_margin_enabled");
    static const StringName s_drag_margin_left("drag_margin_left");
    static const StringName s_drag_margin_top("drag_margin_top");
    static const StringName s_drag_margin_right("drag_margin_right");
    static const StringName s_drag_margin_bottom("drag_margin_bottom");
    static const StringName s_room_width("room_width");
    static const StringName s_room_height("room_height");
    static const StringName s_limit_enabled("limit_enabled");
    static const StringName s_limit_left("limit_left");
    static const StringName s_limit_top("limit_top");
    static const StringName s_limit_right("limit_right");
    static const StringName s_limit_bottom("limit_bottom");

    if (p_name == s_current) { set_current(p_value.as_bool()); return true; }
    if (p_name == s_mode) { mode = static_cast<int>(p_value.as_int()); return true; }
    if (p_name == s_smoothing_enabled) { smoothing_enabled = p_value.as_bool(); return true; }
    if (p_name == s_smoothing_speed) { smoothing_speed = p_value.as_fixed16().to_float(); return true; }
    if (p_name == s_drag_margin_enabled) { drag_margin_enabled = p_value.as_bool(); return true; }
    if (p_name == s_drag_margin_left) { drag_margin_left = p_value.as_fixed16().to_float(); return true; }
    if (p_name == s_drag_margin_top) { drag_margin_top = p_value.as_fixed16().to_float(); return true; }
    if (p_name == s_drag_margin_right) { drag_margin_right = p_value.as_fixed16().to_float(); return true; }
    if (p_name == s_drag_margin_bottom) { drag_margin_bottom = p_value.as_fixed16().to_float(); return true; }
    if (p_name == s_room_width) { room_width = static_cast<int>(p_value.as_int()); return true; }
    if (p_name == s_room_height) { room_height = static_cast<int>(p_value.as_int()); return true; }
    if (p_name == s_limit_enabled) { limit_enabled = p_value.as_bool(); return true; }
    if (p_name == s_limit_left) { limit_left = p_value.as_fixed16().to_float(); return true; }
    if (p_name == s_limit_top) { limit_top = p_value.as_fixed16().to_float(); return true; }
    if (p_name == s_limit_right) { limit_right = p_value.as_fixed16().to_float(); return true; }
    if (p_name == s_limit_bottom) { limit_bottom = p_value.as_fixed16().to_float(); return true; }


    return Node2D::set(p_name, p_value);
}

void Camera2D::set_current(bool p_current) {

    current = p_current;
    if (current) {
        current_camera = this;
    } else if (current_camera == this) {
        current_camera = nullptr;
    }
}

void Camera2D::make_current() {
    set_current(true);
}

void Camera2D::shake(float p_strength, float p_duration) {
    shake_strength = p_strength;
    shake_timer = p_duration;
}

void Camera2D::_process(float delta) {
    if (!current) return;

    int v_w = VisualServer::get()->get_virtual_width();
    int v_h = VisualServer::get()->get_virtual_height();
    Vector2Fixed center_offset = Vector2Fixed::from_floats(v_w / 2.0f, v_h / 2.0f);

    Vector2Fixed target_pos = get_global_position();
    if (parent) {
        Node2D* n2d_parent = dynamic_cast<Node2D*>(parent);
        if (n2d_parent) {
            target_pos = n2d_parent->get_global_position();
        }
    }

    Vector2Fixed current_cam_offset = VisualServer::get()->get_camera_offset();
    Vector2Fixed current_cam_center = current_cam_offset + center_offset;

    Vector2Fixed desired_center = target_pos;

    CameraMode cam_mode = static_cast<CameraMode>(mode);

    if (cam_mode == CameraMode::FOLLOW) {
        if (drag_margin_enabled) {
            float dm_left = static_cast<float>(v_w) * drag_margin_left;
            float dm_right = static_cast<float>(v_w) * (1.0f - drag_margin_right);
            float dm_top = static_cast<float>(v_h) * drag_margin_top;
            float dm_bottom = static_cast<float>(v_h) * (1.0f - drag_margin_bottom);

            float rel_x = target_pos.x.to_float() - current_cam_offset.x.to_float();
            float rel_y = target_pos.y.to_float() - current_cam_offset.y.to_float();

            if (rel_x < dm_left) {
                desired_center.x = Fixed16::from_float(target_pos.x.to_float() - dm_left + v_w * 0.5f);
            } else if (rel_x > dm_right) {
                desired_center.x = Fixed16::from_float(target_pos.x.to_float() - dm_right + v_w * 0.5f);
            } else {
                desired_center.x = current_cam_center.x;
            }

            if (rel_y < dm_top) {
                desired_center.y = Fixed16::from_float(target_pos.y.to_float() - dm_top + v_h * 0.5f);
            } else if (rel_y > dm_bottom) {
                desired_center.y = Fixed16::from_float(target_pos.y.to_float() - dm_bottom + v_h * 0.5f);
            } else {
                desired_center.y = current_cam_center.y;
            }
        }
    } else if (cam_mode == CameraMode::ZELDA_ROOM) {
        int r_w = (room_width > 0) ? room_width : v_w;
        int r_h = (room_height > 0) ? room_height : v_h;

        int room_x = static_cast<int>(std::floor(target_pos.x.to_float() / r_w));
        int room_y = static_cast<int>(std::floor(target_pos.y.to_float() / r_h));

        desired_center = Vector2Fixed::from_floats(
            room_x * r_w + v_w * 0.5f,
            room_y * r_h + v_h * 0.5f
        );
    } else if (cam_mode == CameraMode::SMB1_FORWARD) {
        float target_x = target_pos.x.to_float();
        if (target_x > max_forward_x) {
            max_forward_x = target_x;
        }
        desired_center.x = Fixed16::from_float(max_forward_x);
        desired_center.y = target_pos.y;
    }

    // Smooth Interpolation
    Vector2Fixed final_center = desired_center;
    if (smoothing_enabled && delta > 0.0f) {
        float factor = std::clamp(smoothing_speed * delta, 0.0f, 1.0f);
        float lerp_x = current_cam_center.x.to_float() + (desired_center.x.to_float() - current_cam_center.x.to_float()) * factor;
        float lerp_y = current_cam_center.y.to_float() + (desired_center.y.to_float() - current_cam_center.y.to_float()) * factor;
        final_center = Vector2Fixed::from_floats(lerp_x, lerp_y);
    }

    Vector2Fixed cam_pos = final_center - center_offset;

    // Apply Limits
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

    // Apply Retro Screen Shake Offset
    if (shake_timer > 0.0f) {
        shake_timer -= delta;
        if (shake_timer < 0.0f) shake_timer = 0.0f;
        float shake_offset_x = (static_cast<float>(rand() % 100) / 100.0f - 0.5f) * 2.0f * shake_strength;
        float shake_offset_y = (static_cast<float>(rand() % 100) / 100.0f - 0.5f) * 2.0f * shake_strength;
        cam_pos.x += Fixed16::from_float(shake_offset_x);
        cam_pos.y += Fixed16::from_float(shake_offset_y);
    }

    VisualServer::get()->set_camera_offset(cam_pos);
}

} // namespace RetroNode
