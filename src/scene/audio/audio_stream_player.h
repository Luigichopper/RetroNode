#ifndef RETRONODE_AUDIO_STREAM_PLAYER_H
#define RETRONODE_AUDIO_STREAM_PLAYER_H

#include "../main/node.h"
#include <string>

namespace RetroNode {

class RN_API AudioStreamPlayer : public Node {
    RN_CLASS(AudioStreamPlayer, Node)

public:
    std::string stream_path;
    uint32_t sound_id = 0;
    float volume = 1.0f;
    bool autoplay = false;

    AudioStreamPlayer();
    virtual ~AudioStreamPlayer() = default;

    virtual void get_property_list(std::vector<PropertyInfo>& out_list) const override;
    virtual Variant get(const StringName& p_name) const override;
    virtual bool set(const StringName& p_name, const Variant& p_value) override;

    void set_stream_path(const std::string& path);
    const std::string& get_stream_path() const { return stream_path; }

    void set_volume(float v) { volume = v; }
    float get_volume() const { return volume; }

    void set_autoplay(bool a) { autoplay = a; }
    bool has_autoplay() const { return autoplay; }

    void play();

    virtual void _ready() override;
};

} // namespace RetroNode

#endif // RETRONODE_AUDIO_STREAM_PLAYER_H
