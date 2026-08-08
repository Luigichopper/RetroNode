#ifndef RETRONODE_SPRITE_2D_H
#define RETRONODE_SPRITE_2D_H

#include "node_2d.h"
#include <SDL3/SDL.h>

namespace RetroNode {

class Sprite2D : public Node2D {
    RN_CLASS(Sprite2D, Node2D)

public:
    Vector2Fixed texture_size;
    Rect2Fixed region_rect;
    int z_index = 0;
    SDL_Color modulate = {255, 255, 255, 255};

    Sprite2D();
    virtual ~Sprite2D() = default;

    virtual void _process(float delta) override;
};

class AnimatedSprite2D : public Sprite2D {
    RN_CLASS(AnimatedSprite2D, Sprite2D)

private:
    std::string current_animation = "idle";

public:
    AnimatedSprite2D();
    virtual ~AnimatedSprite2D() = default;

    void play(const std::string& anim_name) {
        current_animation = anim_name;
    }
};

} // namespace RetroNode

#endif // RETRONODE_SPRITE_2D_H
