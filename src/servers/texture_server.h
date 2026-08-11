#ifndef RETRONODE_TEXTURE_SERVER_H
#define RETRONODE_TEXTURE_SERVER_H

#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "../core/object/class_db.h"
#include "../core/math/vector2.h"
#include "../core/math/rect2.h"

namespace RetroNode {

struct TextureData {
    uint32_t id;
    std::string path;
    SDL_Texture* sdl_texture;
    int width;
    int height;
};

struct AtlasRegion {
    uint32_t texture_id;
    Rect2Fixed rect;
};

class RN_API TextureServer {
private:
    static TextureServer* instance;
    std::vector<TextureData> textures;
    std::unordered_map<std::string, uint32_t> path_to_id;
    std::unordered_map<std::string, AtlasRegion> atlas_regions;
    SDL_Renderer* renderer = nullptr;
    std::string project_dir = "";


public:
    TextureServer();
    ~TextureServer();

    static TextureServer* get() {
        if (!instance) {
            instance = new TextureServer();
        }
        return instance;
    }

    void init(SDL_Renderer* p_renderer);
    void shutdown();

    void set_project_dir(const std::string& p_dir) { project_dir = p_dir; }
    const std::string& get_project_dir() const { return project_dir; }

    uint32_t load_texture(const std::string& filepath);
    void set_window_icon(SDL_Window* window, const std::string& filepath);
    uint32_t create_procedural_texture(const std::string& name, int width, int height, const uint8_t* rgba_pixels);


    bool load_atlas_manifest(const std::string& manifest_path);
    bool get_atlas_region(const std::string& region_name, uint32_t& out_texture_id, Rect2Fixed& out_rect) const;

    SDL_Texture* get_texture(uint32_t texture_id) const;
    Vector2Fixed get_texture_size(uint32_t texture_id) const;
    size_t get_texture_count() const { return textures.size() > 0 ? textures.size() - 1 : 0; }
};

} // namespace RetroNode

#endif // RETRONODE_TEXTURE_SERVER_H
