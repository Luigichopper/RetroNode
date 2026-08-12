#include "sprite_frames.h"
#include "../../servers/texture_server.h"
#include <algorithm>

namespace RetroNode {

SpriteFrames::SpriteFrames() {
    add_animation("default");
}

void SpriteFrames::rebuild_index_map() {
    anim_index_map.clear();
    for (size_t i = 0; i < animations.size(); ++i) {
        anim_index_map[animations[i].name] = i;
    }
}

void SpriteFrames::add_animation(const std::string& anim_name) {
    if (anim_name.empty() || has_animation(anim_name)) return;
    SpriteAnimation anim;
    anim.name = anim_name;
    anim.speed = 5.0f;
    anim.loop = true;
    animations.push_back(anim);
    rebuild_index_map();
}

bool SpriteFrames::has_animation(const std::string& anim_name) const {
    return anim_index_map.find(anim_name) != anim_index_map.end();
}

void SpriteFrames::remove_animation(const std::string& anim_name) {
    auto it = anim_index_map.find(anim_name);
    if (it == anim_index_map.end()) return;
    animations.erase(animations.begin() + it->second);
    rebuild_index_map();

    if (animations.empty()) {
        add_animation("default");
    }
}

void SpriteFrames::rename_animation(const std::string& old_name, const std::string& new_name) {
    if (new_name.empty() || old_name == new_name || has_animation(new_name)) return;
    auto it = anim_index_map.find(old_name);
    if (it == anim_index_map.end()) return;
    animations[it->second].name = new_name;
    rebuild_index_map();
}

std::vector<std::string> SpriteFrames::get_animation_names() const {
    std::vector<std::string> names;
    names.reserve(animations.size());
    for (const auto& anim : animations) {
        names.push_back(anim.name);
    }
    return names;
}

void SpriteFrames::set_animation_speed(const std::string& anim_name, float fps) {
    auto it = anim_index_map.find(anim_name);
    if (it == anim_index_map.end()) return;
    animations[it->second].speed = std::max(0.01f, fps);
}

float SpriteFrames::get_animation_speed(const std::string& anim_name) const {
    auto it = anim_index_map.find(anim_name);
    if (it == anim_index_map.end()) return 5.0f;
    return animations[it->second].speed;
}

void SpriteFrames::set_animation_loop(const std::string& anim_name, bool loop) {
    auto it = anim_index_map.find(anim_name);
    if (it == anim_index_map.end()) return;
    animations[it->second].loop = loop;
}

bool SpriteFrames::get_animation_loop(const std::string& anim_name) const {
    auto it = anim_index_map.find(anim_name);
    if (it == anim_index_map.end()) return true;
    return animations[it->second].loop;
}

void SpriteFrames::add_frame(const std::string& anim_name, const std::string& texture_path, const Rect2Fixed& region, float duration) {
    auto it = anim_index_map.find(anim_name);
    if (it == anim_index_map.end()) return;

    AnimationFrame frame;
    frame.texture_path = texture_path;
    frame.duration = std::max(0.01f, duration);
    frame.region_rect = region;

    if (!frame.texture_path.empty()) {
        frame.texture_id = TextureServer::get()->load_texture(frame.texture_path);
        if (frame.region_rect.size.x == Fixed16(0) || frame.region_rect.size.y == Fixed16(0)) {
            Vector2Fixed sz = TextureServer::get()->get_texture_size(frame.texture_id);
            if (sz.x > Fixed16(0) && sz.y > Fixed16(0)) {
                frame.region_rect = Rect2Fixed(Vector2Fixed::zero(), sz);
            }
        }
    }

    animations[it->second].frames.push_back(frame);
}

void SpriteFrames::insert_frame(const std::string& anim_name, int index, const std::string& texture_path, const Rect2Fixed& region, float duration) {
    auto it = anim_index_map.find(anim_name);
    if (it == anim_index_map.end()) return;

    auto& frames = animations[it->second].frames;
    if (index < 0 || index > static_cast<int>(frames.size())) {
        index = static_cast<int>(frames.size());
    }

    AnimationFrame frame;
    frame.texture_path = texture_path;
    frame.duration = std::max(0.01f, duration);
    frame.region_rect = region;

    if (!frame.texture_path.empty()) {
        frame.texture_id = TextureServer::get()->load_texture(frame.texture_path);
        if (frame.region_rect.size.x == Fixed16(0) || frame.region_rect.size.y == Fixed16(0)) {
            Vector2Fixed sz = TextureServer::get()->get_texture_size(frame.texture_id);
            if (sz.x > Fixed16(0) && sz.y > Fixed16(0)) {
                frame.region_rect = Rect2Fixed(Vector2Fixed::zero(), sz);
            }
        }
    }

    frames.insert(frames.begin() + index, frame);
}

void SpriteFrames::remove_frame(const std::string& anim_name, int index) {
    auto it = anim_index_map.find(anim_name);
    if (it == anim_index_map.end()) return;

    auto& frames = animations[it->second].frames;
    if (index >= 0 && index < static_cast<int>(frames.size())) {
        frames.erase(frames.begin() + index);
    }
}

int SpriteFrames::get_frame_count(const std::string& anim_name) const {
    auto it = anim_index_map.find(anim_name);
    if (it == anim_index_map.end()) return 0;
    return static_cast<int>(animations[it->second].frames.size());
}

const AnimationFrame* SpriteFrames::get_frame(const std::string& anim_name, int index) const {
    auto it = anim_index_map.find(anim_name);
    if (it == anim_index_map.end()) return nullptr;

    const auto& frames = animations[it->second].frames;
    if (index >= 0 && index < static_cast<int>(frames.size())) {
        return &frames[index];
    }
    return nullptr;
}

AnimationFrame* SpriteFrames::get_frame_mut(const std::string& anim_name, int index) {
    auto it = anim_index_map.find(anim_name);
    if (it == anim_index_map.end()) return nullptr;

    auto& frames = animations[it->second].frames;
    if (index >= 0 && index < static_cast<int>(frames.size())) {
        return &frames[index];
    }
    return nullptr;
}

void SpriteFrames::set_frame_duration(const std::string& anim_name, int index, float duration) {
    AnimationFrame* f = get_frame_mut(anim_name, index);
    if (f) {
        f->duration = std::max(0.01f, duration);
    }
}

float SpriteFrames::get_frame_duration(const std::string& anim_name, int index) const {
    const AnimationFrame* f = get_frame(anim_name, index);
    return f ? f->duration : 1.0f;
}

void SpriteFrames::move_frame(const std::string& anim_name, int old_index, int new_index) {
    auto it = anim_index_map.find(anim_name);
    if (it == anim_index_map.end()) return;

    auto& frames = animations[it->second].frames;
    int count = static_cast<int>(frames.size());
    if (old_index < 0 || old_index >= count || new_index < 0 || new_index >= count || old_index == new_index) {
        return;
    }

    AnimationFrame temp = frames[old_index];
    frames.erase(frames.begin() + old_index);
    frames.insert(frames.begin() + new_index, temp);
}

void SpriteFrames::clear_animation(const std::string& anim_name) {
    auto it = anim_index_map.find(anim_name);
    if (it == anim_index_map.end()) return;
    animations[it->second].frames.clear();
}

void SpriteFrames::clear_all() {
    animations.clear();
    anim_index_map.clear();
    add_animation("default");
}

const SpriteAnimation* SpriteFrames::get_animation(const std::string& anim_name) const {
    auto it = anim_index_map.find(anim_name);
    if (it == anim_index_map.end()) return nullptr;
    return &animations[it->second];
}

SpriteAnimation* SpriteFrames::get_animation_mut(const std::string& anim_name) {
    auto it = anim_index_map.find(anim_name);
    if (it == anim_index_map.end()) return nullptr;
    return &animations[it->second];
}

nlohmann::json SpriteFrames::to_json() const {
    nlohmann::json j_anims = nlohmann::json::object();
    for (const auto& anim : animations) {
        nlohmann::json j_anim;
        j_anim["speed"] = anim.speed;
        j_anim["loop"] = anim.loop;

        nlohmann::json j_frames = nlohmann::json::array();
        for (const auto& frame : anim.frames) {
            nlohmann::json j_f;
            j_f["texture"] = frame.texture_path;
            j_f["duration"] = frame.duration;
            j_f["region"] = {
                frame.region_rect.position.x.to_float(),
                frame.region_rect.position.y.to_float(),
                frame.region_rect.size.x.to_float(),
                frame.region_rect.size.y.to_float()
            };
            j_frames.push_back(j_f);
        }
        j_anim["frames"] = j_frames;
        j_anims[anim.name] = j_anim;
    }
    return j_anims;
}

void SpriteFrames::from_json(const nlohmann::json& j) {
    clear_all();
    if (!j.is_object()) return;

    animations.clear();
    anim_index_map.clear();

    for (auto& [name, j_anim] : j.items()) {
        SpriteAnimation anim;
        anim.name = name;
        anim.speed = j_anim.value("speed", 5.0f);
        anim.loop = j_anim.value("loop", true);

        if (j_anim.contains("frames") && j_anim["frames"].is_array()) {
            for (const auto& j_f : j_anim["frames"]) {
                AnimationFrame frame;
                frame.texture_path = j_f.value("texture", "");
                frame.duration = j_f.value("duration", 1.0f);

                if (j_f.contains("region") && j_f["region"].is_array() && j_f["region"].size() >= 4) {
                    float x = j_f["region"][0].get<float>();
                    float y = j_f["region"][1].get<float>();
                    float w = j_f["region"][2].get<float>();
                    float h = j_f["region"][3].get<float>();
                    frame.region_rect = Rect2Fixed::from_floats(x, y, w, h);
                }

                if (!frame.texture_path.empty()) {
                    frame.texture_id = TextureServer::get()->load_texture(frame.texture_path);
                    if (frame.region_rect.size.x == Fixed16(0) || frame.region_rect.size.y == Fixed16(0)) {
                        Vector2Fixed sz = TextureServer::get()->get_texture_size(frame.texture_id);
                        if (sz.x > Fixed16(0) && sz.y > Fixed16(0)) {
                            frame.region_rect = Rect2Fixed(Vector2Fixed::zero(), sz);
                        }
                    }
                }
                anim.frames.push_back(frame);
            }
        }
        animations.push_back(anim);
    }

    rebuild_index_map();
    if (animations.empty()) {
        add_animation("default");
    }
}

} // namespace RetroNode
