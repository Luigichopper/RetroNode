#include "sprite_2d.h"
#include "../../servers/visual_server.h"
#include "../../servers/texture_server.h"

namespace RetroNode {

Sprite2D::Sprite2D() 
    : texture_size(Vector2Fixed::from_floats(16.0f, 16.0f)),
      region_rect(Rect2Fixed::from_floats(0.0f, 0.0f, 16.0f, 16.0f)) {
    name = "Sprite2D";
}

void Sprite2D::get_property_list(std::vector<PropertyInfo>& out_list) const {
    Node2D::get_property_list(out_list);
    out_list.push_back({ StringName("texture"), VariantType::STRING, PropertyHint::FILE_PATH, "*.png" });
    out_list.push_back({ StringName("z_index"), VariantType::INT });
    out_list.push_back({ StringName("modulate"), VariantType::COLOR });
}

Variant Sprite2D::get(const StringName& p_name) const {
    static const StringName s_texture("texture");
    static const StringName s_texture_path("texture_path");
    static const StringName s_z_index("z_index");
    static const StringName s_modulate("modulate");

    if (p_name == s_texture || p_name == s_texture_path) return Variant(texture_path);
    if (p_name == s_z_index) return Variant((int64_t)z_index);
    if (p_name == s_modulate) return Variant(modulate);
    return Node2D::get(p_name);
}

bool Sprite2D::set(const StringName& p_name, const Variant& p_value) {
    static const StringName s_texture("texture");
    static const StringName s_texture_path("texture_path");
    static const StringName s_z_index("z_index");
    static const StringName s_modulate("modulate");

    if (p_name == s_texture || p_name == s_texture_path) {
        set_texture_path(p_value.as_string());
        return true;
    }
    if (p_name == s_z_index) {
        z_index = static_cast<int>(p_value.as_int());
        return true;
    }
    if (p_name == s_modulate) {
        modulate = p_value.as_color();
        return true;
    }
    return Node2D::set(p_name, p_value);
}

void Sprite2D::set_texture_path(const std::string& path) {
    texture_path = path;
    if (!texture_path.empty()) {
        texture_id = TextureServer::get()->load_texture(texture_path);
        Vector2Fixed sz = TextureServer::get()->get_texture_size(texture_id);
        if (sz.x > Fixed16(0) && sz.y > Fixed16(0)) {
            texture_size = sz;
            region_rect = Rect2Fixed(Vector2Fixed::zero(), texture_size);
        }
    }
}

void Sprite2D::_process(float delta) {
    (void)delta;
    if (!is_visible_in_tree()) return;
    Vector2Fixed global_pos = get_global_position();
    Vector2Fixed global_prev_pos = get_global_previous_position();

    VisualServer::get()->submit_draw_sprite(
        global_pos,
        global_prev_pos,
        texture_size,
        region_rect,
        texture_id,
        z_index,
        modulate,
        get_global_rotation(),
        get_global_scale()
    );
}

AnimatedSprite2D::AnimatedSprite2D() {
    name = "AnimatedSprite2D";
    modulate = { 255, 255, 255, 255 }; 
}

} // namespace RetroNode
