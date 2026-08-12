#ifndef RETRONODE_ANIMATED_SPRITE_2D_H
#define RETRONODE_ANIMATED_SPRITE_2D_H

#include "node_2d.h"
#include "../../core/resource/sprite_frames.h"
#include <SDL3/SDL.h>
#include <string>
#include <memory>

namespace RetroNode {

class RN_API AnimatedSprite2D : public Node2D {
    RN_CLASS(AnimatedSprite2D, Node2D)

private:
    std::shared_ptr<SpriteFrames> sprite_frames;
    std::string animation = "default";
    std::string autoplay = "";
    int frame = 0;
    float speed_scale = 1.0f;
    float custom_speed = 1.0f;
    bool playing = false;
    bool centered = true;
    Vector2Fixed offset = Vector2Fixed::zero();
    bool flip_h = false;
    bool flip_v = false;
    int z_index = 10;
    SDL_Color modulate = {255, 255, 255, 255};

    Fixed16 frame_accumulator = Fixed16(0);

public:
    AnimatedSprite2D();
    virtual ~AnimatedSprite2D() = default;

    virtual void get_property_list(std::vector<PropertyInfo>& out_list) const override;
    virtual Variant get(const StringName& p_name) const override;
    virtual bool set(const StringName& p_name, const Variant& p_value) override;

    // SpriteFrames resource access
    SpriteFrames* get_sprite_frames() const { return sprite_frames.get(); }
    std::shared_ptr<SpriteFrames> get_sprite_frames_shared() const { return sprite_frames; }
    void set_sprite_frames(std::shared_ptr<SpriteFrames> p_frames);

    // Animation playback
    void play(const std::string& p_anim = "", float p_custom_speed = 1.0f, bool p_from_end = false);
    void pause();
    void stop();
    void set_playing(bool p_playing) { if (p_playing) play(animation); else pause(); }
    bool is_playing() const { return playing; }

    void set_animation(const std::string& p_anim);
    const std::string& get_animation() const { return animation; }

    void set_frame(int p_frame);
    int get_frame() const { return frame; }

    void set_speed_scale(float p_scale) { speed_scale = std::max(0.0f, p_scale); }
    float get_speed_scale() const { return speed_scale; }

    void set_autoplay(const std::string& p_anim) { autoplay = p_anim; }
    const std::string& get_autoplay() const { return autoplay; }

    void set_centered(bool p_centered) { centered = p_centered; }
    bool is_centered() const { return centered; }

    void set_offset(const Vector2Fixed& p_offset) { offset = p_offset; }
    Vector2Fixed get_offset() const { return offset; }

    void set_flip_h(bool p_flip) { flip_h = p_flip; }
    bool is_flip_h() const { return flip_h; }

    void set_flip_v(bool p_flip) { flip_v = p_flip; }
    bool is_flip_v() const { return flip_v; }

    void set_z_index(int p_z) { z_index = p_z; }
    int get_z_index() const { return z_index; }

    void set_modulate(const SDL_Color& p_col) { modulate = p_col; }
    SDL_Color get_modulate() const { return modulate; }

    virtual void _ready() override;
    virtual void _physics_process(Fixed16 delta) override;
    virtual void _process(float delta) override;
};

} // namespace RetroNode

#endif // RETRONODE_ANIMATED_SPRITE_2D_H
