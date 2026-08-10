#ifndef RETRONODE_SPRITE_2D_H
#define RETRONODE_SPRITE_2D_H

#include "node_2d.h"
#include <SDL3/SDL.h>

namespace RetroNode {

class RN_API Sprite2D : public Node2D {
    RN_CLASS(Sprite2D, Node2D)

public:
    uint32_t texture_id = 0;
    std::string texture_path;
    Vector2Fixed texture_size;
    Rect2Fixed region_rect;
    int z_index = 10;
    SDL_Color modulate = {255, 255, 255, 255};

    Sprite2D();
    virtual ~Sprite2D() = default;

    virtual void get_property_list(std::vector<PropertyInfo>& out_list) const override;
    virtual Variant get(const StringName& p_name) const override;
    virtual bool set(const StringName& p_name, const Variant& p_value) override;

    void set_texture_path(const std::string& path);
    const std::string& get_texture_path() const { return texture_path; }
    virtual void _process(float delta) override;
};

class RN_API AnimatedSprite2D : public Sprite2D {
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
