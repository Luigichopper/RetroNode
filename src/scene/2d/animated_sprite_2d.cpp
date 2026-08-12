#include "animated_sprite_2d.h"
#include "../../servers/visual_server.h"
#include "../../servers/texture_server.h"
#include <algorithm>

namespace RetroNode {

AnimatedSprite2D::AnimatedSprite2D() {
    name = "AnimatedSprite2D";
    sprite_frames = std::make_shared<SpriteFrames>();
}

void AnimatedSprite2D::get_property_list(std::vector<PropertyInfo>& out_list) const {
    Node2D::get_property_list(out_list);
    out_list.push_back({ StringName("animation"), VariantType::STRING });
    out_list.push_back({ StringName("autoplay"), VariantType::STRING });
    out_list.push_back({ StringName("frame"), VariantType::INT });
    out_list.push_back({ StringName("speed_scale"), VariantType::FLOAT16 });
    out_list.push_back({ StringName("playing"), VariantType::BOOL });
    out_list.push_back({ StringName("centered"), VariantType::BOOL });
    out_list.push_back({ StringName("offset"), VariantType::VECTOR2 });
    out_list.push_back({ StringName("flip_h"), VariantType::BOOL });
    out_list.push_back({ StringName("flip_v"), VariantType::BOOL });
    out_list.push_back({ StringName("z_index"), VariantType::INT });
    out_list.push_back({ StringName("modulate"), VariantType::COLOR });
}

Variant AnimatedSprite2D::get(const StringName& p_name) const {
    static const StringName s_animation("animation");
    static const StringName s_autoplay("autoplay");
    static const StringName s_frame("frame");
    static const StringName s_speed_scale("speed_scale");
    static const StringName s_playing("playing");
    static const StringName s_centered("centered");
    static const StringName s_offset("offset");
    static const StringName s_flip_h("flip_h");
    static const StringName s_flip_v("flip_v");
    static const StringName s_z_index("z_index");
    static const StringName s_modulate("modulate");

    if (p_name == s_animation) return Variant(animation);
    if (p_name == s_autoplay) return Variant(autoplay);
    if (p_name == s_frame) return Variant((int64_t)frame);
    if (p_name == s_speed_scale) return Variant(Fixed16::from_float(speed_scale));
    if (p_name == s_playing) return Variant(playing);
    if (p_name == s_centered) return Variant(centered);
    if (p_name == s_offset) return Variant(offset);
    if (p_name == s_flip_h) return Variant(flip_h);
    if (p_name == s_flip_v) return Variant(flip_v);
    if (p_name == s_z_index) return Variant((int64_t)z_index);
    if (p_name == s_modulate) return Variant(modulate);

    return Node2D::get(p_name);
}

bool AnimatedSprite2D::set(const StringName& p_name, const Variant& p_value) {
    static const StringName s_animation("animation");
    static const StringName s_autoplay("autoplay");
    static const StringName s_frame("frame");
    static const StringName s_speed_scale("speed_scale");
    static const StringName s_playing("playing");
    static const StringName s_centered("centered");
    static const StringName s_offset("offset");
    static const StringName s_flip_h("flip_h");
    static const StringName s_flip_v("flip_v");
    static const StringName s_z_index("z_index");
    static const StringName s_modulate("modulate");

    if (p_name == s_animation) {
        set_animation(p_value.as_string());
        return true;
    }
    if (p_name == s_autoplay) {
        set_autoplay(p_value.as_string());
        return true;
    }
    if (p_name == s_frame) {
        set_frame(static_cast<int>(p_value.as_int()));
        return true;
    }
    if (p_name == s_speed_scale) {
        set_speed_scale(p_value.as_fixed16().to_float());
        return true;
    }
    if (p_name == s_playing) {
        if (p_value.as_bool()) play(animation);
        else pause();
        return true;
    }
    if (p_name == s_centered) {
        set_centered(p_value.as_bool());
        return true;
    }
    if (p_name == s_offset) {
        set_offset(p_value.as_vector2());
        return true;
    }
    if (p_name == s_flip_h) {
        set_flip_h(p_value.as_bool());
        return true;
    }
    if (p_name == s_flip_v) {
        set_flip_v(p_value.as_bool());
        return true;
    }
    if (p_name == s_z_index) {
        set_z_index(static_cast<int>(p_value.as_int()));
        return true;
    }
    if (p_name == s_modulate) {
        set_modulate(p_value.as_color());
        return true;
    }

    return Node2D::set(p_name, p_value);
}

void AnimatedSprite2D::set_sprite_frames(std::shared_ptr<SpriteFrames> p_frames) {
    if (p_frames) {
        sprite_frames = p_frames;
    } else {
        sprite_frames = std::make_shared<SpriteFrames>();
    }
    frame = 0;
    frame_accumulator = Fixed16(0);
}

void AnimatedSprite2D::set_animation(const std::string& p_anim) {
    if (animation == p_anim) return;
    animation = p_anim;
    if (sprite_frames && !sprite_frames->has_animation(animation)) {
        sprite_frames->add_animation(animation);
    }
    frame = 0;
    frame_accumulator = Fixed16(0);
}

void AnimatedSprite2D::set_frame(int p_frame) {
    frame = p_frame;
    int count = sprite_frames ? sprite_frames->get_frame_count(animation) : 0;
    if (count > 0) {
        if (frame < 0) frame = 0;
        if (frame >= count) frame = count - 1;
    } else {
        frame = 0;
    }
    frame_accumulator = Fixed16(0);
}

void AnimatedSprite2D::play(const std::string& p_anim, float p_custom_speed, bool p_from_end) {
    if (!p_anim.empty()) {
        if (animation != p_anim) {
            animation = p_anim;
            if (sprite_frames && !sprite_frames->has_animation(animation)) {
                sprite_frames->add_animation(animation);
            }
            frame = p_from_end ? std::max(0, sprite_frames->get_frame_count(animation) - 1) : 0;
            frame_accumulator = Fixed16(0);
        }
    }
    custom_speed = p_custom_speed;
    playing = true;
}

void AnimatedSprite2D::pause() {
    playing = false;
}

void AnimatedSprite2D::stop() {
    playing = false;
    frame = 0;
    frame_accumulator = Fixed16(0);
}

void AnimatedSprite2D::_ready() {
    if (!autoplay.empty()) {
        play(autoplay);
    }
}

void AnimatedSprite2D::_physics_process(Fixed16 delta) {
    if (!playing || !sprite_frames) return;

    int total_frames = sprite_frames->get_frame_count(animation);
    if (total_frames <= 0) return;

    float base_fps = sprite_frames->get_animation_speed(animation);
    float frame_dur = sprite_frames->get_frame_duration(animation, frame);
    float effective_fps = base_fps * custom_speed * speed_scale / std::max(0.01f, frame_dur);

    if (effective_fps <= 0.001f) return;

    float step_sec = 1.0f / effective_fps;
    Fixed16 step_fixed = Fixed16::from_float(step_sec);
    if (step_fixed <= Fixed16(0)) return;

    frame_accumulator += delta;

    while (frame_accumulator >= step_fixed) {
        frame_accumulator -= step_fixed;
        frame++;

        if (frame >= total_frames) {
            bool loop = sprite_frames->get_animation_loop(animation);
            if (loop) {
                frame = 0;
            } else {
                frame = total_frames - 1;
                playing = false;
                frame_accumulator = Fixed16(0);
                break;
            }
        }
    }
}

void AnimatedSprite2D::_process(float delta) {
    (void)delta;
    if (!is_visible_in_tree() || !sprite_frames) return;

    const AnimationFrame* anim_frame = sprite_frames->get_frame(animation, frame);
    if (!anim_frame || anim_frame->texture_path.empty()) return;

    uint32_t tex_id = anim_frame->texture_id;
    if (tex_id == 0) {
        tex_id = TextureServer::get()->load_texture(anim_frame->texture_path);
    }
    if (tex_id == 0) return;

    Rect2Fixed src_rect = anim_frame->region_rect;
    Vector2Fixed draw_size = src_rect.size;
    if (draw_size.x <= Fixed16(0) || draw_size.y <= Fixed16(0)) {
        draw_size = TextureServer::get()->get_texture_size(tex_id);
        src_rect = Rect2Fixed(Vector2Fixed::zero(), draw_size);
    }

    Vector2Fixed global_pos = get_global_position();
    Vector2Fixed global_prev_pos = get_global_previous_position();

    Vector2Fixed render_pos = global_pos;
    Vector2Fixed render_prev_pos = global_prev_pos;

    if (centered) {
        Vector2Fixed half_size = draw_size * Fixed16::from_float(0.5f);
        render_pos = render_pos - half_size;
        render_prev_pos = render_prev_pos - half_size;
    }

    render_pos = render_pos + offset;
    render_prev_pos = render_prev_pos + offset;

    if (get_canvas_layer()) {
        Vector2Fixed cam_offset = VisualServer::get()->get_camera_offset();
        render_pos = render_pos + cam_offset;
        render_prev_pos = render_prev_pos + cam_offset;
    }

    VisualServer::get()->submit_draw_sprite(
        render_pos,
        render_prev_pos,
        draw_size,
        src_rect,
        tex_id,
        z_index,
        modulate,
        get_global_rotation(),
        get_global_scale(),
        flip_h,
        flip_v
    );
}

} // namespace RetroNode
