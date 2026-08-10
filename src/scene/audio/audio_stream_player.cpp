#include "audio_stream_player.h"
#include "../../servers/audio_server.h"

namespace RetroNode {

AudioStreamPlayer::AudioStreamPlayer() {
    name = "AudioStreamPlayer";
}

void AudioStreamPlayer::get_property_list(std::vector<PropertyInfo>& out_list) const {
    Node::get_property_list(out_list);
    out_list.push_back({ StringName("stream"), VariantType::STRING, PropertyHint::FILE_PATH, "*.wav" });
    out_list.push_back({ StringName("volume"), VariantType::FLOAT16 });
    out_list.push_back({ StringName("autoplay"), VariantType::BOOL });
}

Variant AudioStreamPlayer::get(const StringName& p_name) const {
    if (p_name == StringName("stream") || p_name == StringName("stream_path")) return Variant(stream_path);
    if (p_name == StringName("volume")) return Variant(Fixed16::from_float(volume));
    if (p_name == StringName("autoplay")) return Variant(autoplay);
    return Node::get(p_name);
}

bool AudioStreamPlayer::set(const StringName& p_name, const Variant& p_value) {
    if (p_name == StringName("stream") || p_name == StringName("stream_path")) {
        set_stream_path(p_value.as_string());
        return true;
    }
    if (p_name == StringName("volume")) {
        volume = p_value.as_fixed16().to_float();
        return true;
    }
    if (p_name == StringName("autoplay")) {
        autoplay = p_value.as_bool();
        return true;
    }
    return Node::set(p_name, p_value);
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
