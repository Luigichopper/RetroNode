#include "animation_player.h"
#include "../../core/object/object_db.h"
#include <iostream>

namespace RetroNode {

AnimationPlayer::AnimationPlayer() {
    name = "AnimationPlayer";
}

void AnimationPlayer::add_track(const AnimationTrack& track) {
    tracks[track.name] = track;
}

bool AnimationPlayer::has_animation(const std::string& anim_name) const {
    return tracks.find(anim_name) != tracks.end();
}

Sprite2D* AnimationPlayer::resolve_target_sprite() {
    if (target_sprite_id != 0) {
        Object* obj = ObjectDB::get()->get_object(target_sprite_id);
        if (obj) {
            Sprite2D* spr = dynamic_cast<Sprite2D*>(obj);
            if (spr) return spr;
        }
        target_sprite_id = 0;
    }

    if (get_parent()) {
        Sprite2D* spr = dynamic_cast<Sprite2D*>(get_parent());
        if (!spr) {
            for (Node* sibling : get_parent()->get_children()) {
                Sprite2D* s = dynamic_cast<Sprite2D*>(sibling);
                if (s) {
                    spr = s;
                    break;
                }
            }
        }
        if (spr) {
            target_sprite_id = spr->get_instance_id();
            return spr;
        }
    }
    return nullptr;
}

void AnimationPlayer::_ready() {
    resolve_target_sprite();
}

void AnimationPlayer::play(const std::string& anim_name) {
    if (current_animation == anim_name && is_playing_flag) {
        return;
    }

    auto it = tracks.find(anim_name);
    if (it == tracks.end()) {
        return;
    }

    current_animation = anim_name;
    current_frame_index = 0;
    frame_timer = 0.0f;
    is_playing_flag = true;

    Sprite2D* target = resolve_target_sprite();
    if (target && !it->second.frames.empty()) {
        target->region_rect = it->second.frames[0];
    }
}

void AnimationPlayer::stop() {
    is_playing_flag = false;
}

void AnimationPlayer::_physics_process(Fixed16 delta) {
    Node::_physics_process(delta);

    if (!is_playing_flag || current_animation.empty()) {
        return;
    }

    auto it = tracks.find(current_animation);
    if (it == tracks.end()) {
        is_playing_flag = false;
        return;
    }

    const auto& track = it->second;
    if (track.frames.empty()) return;

    float step = delta.to_float();
    float frame_duration = (track.fps > 0.0f) ? (1.0f / track.fps) : 0.125f;
    frame_timer += step;

    while (frame_timer >= frame_duration) {
        frame_timer -= frame_duration;
        current_frame_index++;

        if (current_frame_index >= static_cast<int>(track.frames.size())) {
            if (track.loop) {
                current_frame_index = 0;
            } else {
                current_frame_index = static_cast<int>(track.frames.size()) - 1;
                is_playing_flag = false;
                break;
            }
        }
    }

    Sprite2D* target = resolve_target_sprite();
    if (target && current_frame_index < static_cast<int>(track.frames.size())) {
        target->region_rect = track.frames[current_frame_index];
    }
}

} // namespace RetroNode
