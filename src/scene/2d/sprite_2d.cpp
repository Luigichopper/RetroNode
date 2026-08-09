#include "sprite_2d.h"
#include "../../servers/visual_server.h"
#include "../../servers/texture_server.h"

namespace RetroNode {

Sprite2D::Sprite2D() 
    : texture_size(Vector2Fixed::from_floats(16.0f, 16.0f)),
      region_rect(Rect2Fixed::from_floats(0.0f, 0.0f, 16.0f, 16.0f)) {
    name = "Sprite2D";
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
        modulate
    );
}

AnimatedSprite2D::AnimatedSprite2D() {
    name = "AnimatedSprite2D";
    modulate = { 255, 255, 255, 255 }; 
}

} // namespace RetroNode
