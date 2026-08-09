#include "animation_player.h"
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

void AnimationPlayer::_ready() {
    // Find target Sprite2D or AnimatedSprite2D sibling / parent
    if (get_parent()) {
        target_sprite = dynamic_cast<Sprite2D*>(get_parent());
        if (!target_sprite) {
            for (Node* sibling : get_parent()->get_children()) {
                Sprite2D* spr = dynamic_cast<Sprite2D*>(sibling);
                if (spr) {
                    target_sprite = spr;
                    break;
                }
            }
        }
    }
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

    if (target_sprite && !it->second.frames.empty()) {
        target_sprite->region_rect = it->second.frames[0];
    }
}

void AnimationPlayer::stop() {
    is_playing_flag = false;
}

void AnimationPlayer::_process(float delta) {
    Node::_process(delta);

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

    float frame_duration = (track.fps > 0.0f) ? (1.0f / track.fps) : 0.125f;
    frame_timer += delta;

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

    if (target_sprite && current_frame_index < static_cast<int>(track.frames.size())) {
        target_sprite->region_rect = track.frames[current_frame_index];
    }
}

} // namespace RetroNode
