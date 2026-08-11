#ifndef RETRONODE_ANIMATION_PLAYER_H
#define RETRONODE_ANIMATION_PLAYER_H

#include "../main/node.h"
#include "../2d/sprite_2d.h"
#include "../../core/math/rect2.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace RetroNode {

struct AnimationTrack {
    std::string name;
    float fps = 8.0f;
    bool loop = true;
    std::vector<Rect2Fixed> frames;
};

class RN_API AnimationPlayer : public Node {
    RN_CLASS(AnimationPlayer, Node)

private:
    std::unordered_map<std::string, AnimationTrack> tracks;
    std::string current_animation;
    int current_frame_index = 0;
    float frame_timer = 0.0f;
    bool is_playing_flag = false;
    uint64_t target_sprite_id = 0;

    Sprite2D* resolve_target_sprite();

public:
    AnimationPlayer();
    virtual ~AnimationPlayer() = default;

    void add_track(const AnimationTrack& track);
    bool has_animation(const std::string& name) const;

    void play(const std::string& anim_name);
    void stop();
    bool is_playing() const { return is_playing_flag; }
    const std::string& get_current_animation() const { return current_animation; }

    virtual void _ready() override;
    virtual void _physics_process(Fixed16 delta) override;
};

} // namespace RetroNode

#endif // RETRONODE_ANIMATION_PLAYER_H
