#ifndef RETRONODE_AUDIO_SERVER_H
#define RETRONODE_AUDIO_SERVER_H

#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "../core/object/class_db.h"

namespace RetroNode {

struct SoundSample {
    uint32_t id;
    std::string path;
    uint8_t* wav_buffer = nullptr;
    uint32_t wav_length = 0;
    SDL_AudioSpec spec;
};

class RN_API AudioServer {
private:
    static AudioServer* instance;
    SDL_AudioDeviceID audio_device = 0;
    SDL_AudioStream* stream = nullptr;

    std::vector<SoundSample> samples;
    std::unordered_map<std::string, uint32_t> path_to_id;
    std::string project_dir = "./MyRPG";

public:
    AudioServer();
    ~AudioServer();

    static AudioServer* get() {
        if (!instance) {
            instance = new AudioServer();
        }
        return instance;
    }

    void init();
    void shutdown();

    void set_project_dir(const std::string& p_dir) { project_dir = p_dir; }
    const std::string& get_project_dir() const { return project_dir; }

    uint32_t load_sound(const std::string& filepath);
    void play_sound(uint32_t sound_id, float volume = 1.0f);
};

} // namespace RetroNode

#endif // RETRONODE_AUDIO_SERVER_H
