#include "audio_stream_player.h"
#include "../../servers/audio_server.h"

namespace RetroNode {

AudioStreamPlayer::AudioStreamPlayer() {
    name = "AudioStreamPlayer";
}

void AudioStreamPlayer::set_stream_path(const std::string& path) {
    stream_path = path;
    if (!stream_path.empty()) {
        sound_id = AudioServer::get()->load_sound(stream_path);
    }
}

void AudioStreamPlayer::play() {
    if (sound_id == 0 && !stream_path.empty()) {
        sound_id = AudioServer::get()->load_sound(stream_path);
    }
    if (sound_id != 0) {
        AudioServer::get()->play_sound(sound_id, volume);
    }
}

void AudioStreamPlayer::_ready() {
    if (autoplay) {
        play();
    }
}

} // namespace RetroNode
