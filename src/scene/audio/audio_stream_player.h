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

    void set_stream_path(const std::string& path);
    void play();

    virtual void _ready() override;
};

} // namespace RetroNode

#endif // RETRONODE_AUDIO_STREAM_PLAYER_H
