#ifndef RETRONODE_SPRITE_FRAMES_H
#define RETRONODE_SPRITE_FRAMES_H

#include "../object/class_db.h"
#include "../math/rect2.h"
#include "../math/vector2.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace RetroNode {

struct AnimationFrame {
    std::string texture_path;
    uint32_t texture_id = 0;
    Rect2Fixed region_rect; // Position and size within texture (empty size means full texture)
    float duration = 1.0f;  // Relative frame duration multiplier (default 1.0)
};

struct SpriteAnimation {
    std::string name = "default";
    float speed = 5.0f;     // Frames per second
    bool loop = true;       // Whether animation loops
    std::vector<AnimationFrame> frames;
};

class RN_API SpriteFrames : public Object {
    RN_CLASS(SpriteFrames, Object)

private:
    std::vector<SpriteAnimation> animations;
    std::unordered_map<std::string, size_t> anim_index_map;

    void rebuild_index_map();

public:
    SpriteFrames();
    virtual ~SpriteFrames() = default;

    // Animation Management
    void add_animation(const std::string& anim_name);
    bool has_animation(const std::string& anim_name) const;
    void remove_animation(const std::string& anim_name);
    void rename_animation(const std::string& old_name, const std::string& new_name);
    std::vector<std::string> get_animation_names() const;

    // Animation Properties
    void set_animation_speed(const std::string& anim_name, float fps);
    float get_animation_speed(const std::string& anim_name) const;

    void set_animation_loop(const std::string& anim_name, bool loop);
    bool get_animation_loop(const std::string& anim_name) const;

    // Frame Management
    void add_frame(const std::string& anim_name, const std::string& texture_path, const Rect2Fixed& region = Rect2Fixed(), float duration = 1.0f);
    void insert_frame(const std::string& anim_name, int index, const std::string& texture_path, const Rect2Fixed& region = Rect2Fixed(), float duration = 1.0f);
    void remove_frame(const std::string& anim_name, int index);
    int get_frame_count(const std::string& anim_name) const;

    const AnimationFrame* get_frame(const std::string& anim_name, int index) const;
    AnimationFrame* get_frame_mut(const std::string& anim_name, int index);

    void set_frame_duration(const std::string& anim_name, int index, float duration);
    float get_frame_duration(const std::string& anim_name, int index) const;

    void move_frame(const std::string& anim_name, int old_index, int new_index);
    void clear_animation(const std::string& anim_name);
    void clear_all();

    const SpriteAnimation* get_animation(const std::string& anim_name) const;
    SpriteAnimation* get_animation_mut(const std::string& anim_name);

    // Serialization
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
};

} // namespace RetroNode

#endif // RETRONODE_SPRITE_FRAMES_H
