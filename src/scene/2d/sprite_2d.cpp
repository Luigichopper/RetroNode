#include "sprite_2d.h"
#include "../../servers/visual_server.h"

namespace RetroNode {

Sprite2D::Sprite2D() 
    : texture_size(Vector2Fixed::from_floats(16.0f, 16.0f)),
      region_rect(Rect2Fixed::from_floats(0.0f, 0.0f, 16.0f, 16.0f)) {
    name = "Sprite2D";
}

void Sprite2D::_process(float delta) {
    (void)delta;
    Vector2Fixed global_pos = get_global_position();
    VisualServer::get()->submit_draw_sprite(
        global_pos,
        texture_size,
        region_rect,
        z_index,
        modulate
    );
}

AnimatedSprite2D::AnimatedSprite2D() {
    name = "AnimatedSprite2D";
    // Distinctive color tint for AnimatedSprite fallback rendering
    modulate = { 80, 200, 120, 255 }; 
}

} // namespace RetroNode
