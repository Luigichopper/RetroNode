#include "audio_server.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

namespace RetroNode {

AudioServer* AudioServer::instance = nullptr;

AudioServer::AudioServer() {
    samples.push_back({0, "", nullptr, 0, {}});
}

AudioServer::~AudioServer() {
    shutdown();
}

void AudioServer::init() {
    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        if (!SDL_Init(SDL_INIT_AUDIO)) {
            std::cerr << "[AudioServer] Error: Failed to initialize SDL3 Audio: " << SDL_GetError() << std::endl;
            return;
        }
    }

    audio_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (!audio_device) {
        std::cerr << "[AudioServer] Warning: Failed to open audio device: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_ResumeAudioDevice(audio_device);
    std::cout << "[AudioServer] Successfully initialized audio playback device [" << audio_device << "]." << std::endl;
}

void AudioServer::shutdown() {
    if (stream) {
        SDL_UnbindAudioStream(stream);
        SDL_DestroyAudioStream(stream);
        stream = nullptr;
    }
    if (audio_device) {
        SDL_CloseAudioDevice(audio_device);
        audio_device = 0;
    }

    for (auto& s : samples) {
        if (s.wav_buffer) {
            SDL_free(s.wav_buffer);
            s.wav_buffer = nullptr;
        }
    }
    samples.clear();
    path_to_id.clear();
    samples.push_back({0, "", nullptr, 0, {}});
}

uint32_t AudioServer::load_sound(const std::string& filepath) {
    auto it = path_to_id.find(filepath);
    if (it != path_to_id.end()) {
        return it->second;
    }

    std::string resolved_path = filepath;
    if (resolved_path.rfind("res://", 0) == 0) {
        resolved_path = resolved_path.substr(6);
    }

    std::vector<std::string> candidates = {
        project_dir + "/" + resolved_path,
        "./" + resolved_path,
        "./MyRPG/" + resolved_path,
        "../MyRPG/" + resolved_path,
        filepath
    };

    std::string final_path = "";
    for (const auto& cand : candidates) {
        if (fs::exists(cand) && !fs::is_directory(cand)) {
            final_path = cand;
            break;
        }
    }

    if (final_path.empty()) {
        std::cerr << "[AudioServer] Warning: Could not locate audio file '" << filepath << "'" << std::endl;
        return 0;
    }

    SDL_AudioSpec spec;
    uint8_t* wav_buf = nullptr;
    uint32_t wav_len = 0;

    if (!SDL_LoadWAV(final_path.c_str(), &spec, &wav_buf, &wav_len)) {
        std::cerr << "[AudioServer] Warning: Failed to load WAV sound '" << final_path << "': " << SDL_GetError() << std::endl;
        return 0;
    }

    uint32_t new_id = static_cast<uint32_t>(samples.size());
    samples.push_back({new_id, filepath, wav_buf, wav_len, spec});
    path_to_id[filepath] = new_id;

    std::cout << "[AudioServer] Loaded sound sample [" << new_id << "]: " << filepath << " -> " << final_path << " (" << wav_len << " bytes, " << spec.freq << "Hz " << static_cast<int>(spec.channels) << "ch)" << std::endl;
    return new_id;
}

void AudioServer::play_sound(uint32_t sound_id, float volume) {
    if (sound_id == 0 || sound_id >= samples.size()) return;
    if (!audio_device) return;

    const auto& s = samples[sound_id];
    if (!s.wav_buffer || s.wav_length == 0) return;

    SDL_AudioSpec dst_spec;
    if (!SDL_GetAudioDeviceFormat(audio_device, &dst_spec, NULL)) {
        dst_spec.format = SDL_AUDIO_S16LE;
        dst_spec.channels = 2;
        dst_spec.freq = 44100;
    }

    SDL_AudioStream* sound_stream = SDL_CreateAudioStream(&s.spec, &dst_spec);
    if (!sound_stream) {
        std::cerr << "[AudioServer] Error creating sound stream: " << SDL_GetError() << std::endl;
        return;
    }

    if (!SDL_BindAudioStream(audio_device, sound_stream)) {
        std::cerr << "[AudioServer] Error binding sound stream: " << SDL_GetError() << std::endl;
        SDL_DestroyAudioStream(sound_stream);
        return;
    }

    if (volume < 1.0f && volume >= 0.0f) {
        std::vector<uint8_t> temp_buf(s.wav_buffer, s.wav_buffer + s.wav_length);
        if (s.spec.format == SDL_AUDIO_S16LE) {
            int16_t* samples_ptr = reinterpret_cast<int16_t*>(temp_buf.data());
            size_t sample_count = s.wav_length / sizeof(int16_t);
            for (size_t i = 0; i < sample_count; ++i) {
                samples_ptr[i] = static_cast<int16_t>(samples_ptr[i] * volume);
            }
        }
        SDL_PutAudioStreamData(sound_stream, temp_buf.data(), static_cast<int>(temp_buf.size()));
    } else {
        SDL_PutAudioStreamData(sound_stream, s.wav_buffer, static_cast<int>(s.wav_length));
    }

    SDL_FlushAudioStream(sound_stream);
    std::cout << "[AudioServer] Playing sound [" << sound_id << "]: " << s.path << std::endl;
}

} // namespace RetroNode
