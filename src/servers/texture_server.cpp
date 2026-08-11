#include "texture_server.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace RetroNode {

TextureServer* TextureServer::instance = nullptr;

TextureServer::TextureServer() {
    textures.push_back({0, "", nullptr, 0, 0});
}

TextureServer::~TextureServer() {
    shutdown();
}

void TextureServer::init(SDL_Renderer* p_renderer) {
    renderer = p_renderer;
}

void TextureServer::shutdown() {
    for (auto& tex : textures) {
        if (tex.sdl_texture) {
            SDL_DestroyTexture(tex.sdl_texture);
            tex.sdl_texture = nullptr;
        }
    }
    textures.clear();
    path_to_id.clear();
    atlas_regions.clear();
    textures.push_back({0, "", nullptr, 0, 0});
}

uint32_t TextureServer::load_texture(const std::string& filepath) {
    auto it = path_to_id.find(filepath);
    if (it != path_to_id.end()) {
        return it->second;
    }

    if (!renderer) {
        std::cerr << "[TextureServer] Error: Renderer not initialized when loading " << filepath << std::endl;
        return 0;
    }

    // Resolve res:// scheme
    std::string resolved_path = filepath;
    if (resolved_path.rfind("res://", 0) == 0) {
        resolved_path = resolved_path.substr(6);
    }

    std::vector<std::string> candidates;
    if (!project_dir.empty()) {
        candidates.push_back(project_dir + "/" + resolved_path);
    }
    candidates.push_back("./" + resolved_path);
    candidates.push_back(filepath);

    std::string final_path = "";
    for (const auto& cand : candidates) {
        std::error_code ec;
        if (fs::exists(cand, ec) && !fs::is_directory(cand, ec)) {
            final_path = fs::weakly_canonical(cand, ec).string();
            break;
        }
    }

    if (final_path.empty()) {
        std::cerr << "[TextureServer] Warning: Could not locate image resource '" << filepath << "'" << std::endl;
        path_to_id[filepath] = 0;
        return 0;
    }

    int width = 0, height = 0, channels = 0;
    unsigned char* data = stbi_load(final_path.c_str(), &width, &height, &channels, 4);

    if (!data) {
        std::cerr << "[TextureServer] Warning: Failed to load image '" << final_path << "': " << stbi_failure_reason() << std::endl;
        return 0;
    }

    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        width,
        height,
        SDL_PIXELFORMAT_RGBA32,
        data,
        width * 4
    );

    if (!surface) {
        std::cerr << "[TextureServer] Error creating SDL_Surface: " << SDL_GetError() << std::endl;
        stbi_image_free(data);
        return 0;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    stbi_image_free(data);

    if (!texture) {
        std::cerr << "[TextureServer] Error creating SDL_Texture: " << SDL_GetError() << std::endl;
        return 0;
    }

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    uint32_t new_id = static_cast<uint32_t>(textures.size());
    textures.push_back({new_id, filepath, texture, width, height});
    path_to_id[filepath] = new_id;

    std::cout << "[TextureServer] Loaded texture [" << new_id << "]: " << filepath << " -> " << final_path << " (" << width << "x" << height << ")" << std::endl;
    return new_id;
}

void TextureServer::set_window_icon(SDL_Window* window, const std::string& filepath) {
    if (!window || filepath.empty()) return;

    std::string resolved = filepath;
    if (resolved.rfind("res://", 0) == 0) {
        resolved = resolved.substr(6);
    }
    std::string final_path = resolved;
    if (!project_dir.empty() && fs::exists(project_dir + "/" + resolved)) {
        final_path = project_dir + "/" + resolved;
    } else if (fs::exists("./" + resolved)) {
        final_path = "./" + resolved;
    }

    if (!fs::exists(final_path)) return;

    int width = 0, height = 0, channels = 0;
    unsigned char* data = stbi_load(final_path.c_str(), &width, &height, &channels, 4);
    if (!data) return;

    SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA8888, data, width * 4);
    if (surface) {
        SDL_SetWindowIcon(window, surface);
        SDL_DestroySurface(surface);
    }
    stbi_image_free(data);
}

uint32_t TextureServer::create_procedural_texture(const std::string& name, int width, int height, const uint8_t* rgba_pixels) {

    auto it = path_to_id.find(name);
    if (it != path_to_id.end()) {
        return it->second;
    }

    if (!renderer) return 0;

    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        width,
        height,
        SDL_PIXELFORMAT_RGBA32,
        const_cast<uint8_t*>(rgba_pixels),
        width * 4
    );

    if (!surface) return 0;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);

    if (!texture) return 0;

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    uint32_t new_id = static_cast<uint32_t>(textures.size());
    textures.push_back({new_id, name, texture, width, height});
    path_to_id[name] = new_id;

    std::cout << "[TextureServer] Created procedural texture [" << new_id << "]: " << name << " (" << width << "x" << height << ")" << std::endl;
    return new_id;
}

bool TextureServer::load_atlas_manifest(const std::string& manifest_path) {
    std::string resolved_path = manifest_path;
    if (resolved_path.rfind("res://", 0) == 0) {
        resolved_path = resolved_path.substr(6);
    }

    std::string final_path = project_dir + "/" + resolved_path;
    if (!fs::exists(final_path)) {
        final_path = manifest_path;
    }

    std::ifstream file(final_path);
    if (!file.is_open()) return false;

    try {
        json j;
        file >> j;

        uint32_t atlas_tex_id = load_texture("res://assets/atlas.png");

        if (j.contains("regions") && j["regions"].is_object()) {
            for (auto& [reg_name, reg_data] : j["regions"].items()) {
                float rx = reg_data.value("x", 0.0f);
                float ry = reg_data.value("y", 0.0f);
                float rw = reg_data.value("w", 16.0f);
                float rh = reg_data.value("h", 16.0f);

                AtlasRegion reg = {
                    atlas_tex_id,
                    Rect2Fixed::from_floats(rx, ry, rw, rh)
                };
                atlas_regions[reg_name] = reg;
            }
            std::cout << "[TextureServer] Loaded atlas manifest: " << final_path << " (" << atlas_regions.size() << " regions)" << std::endl;
            return true;
        }
    } catch (const std::exception& e) {
        std::cerr << "[TextureServer] Error parsing atlas JSON: " << e.what() << std::endl;
    }
    return false;
}

bool TextureServer::get_atlas_region(const std::string& region_name, uint32_t& out_texture_id, Rect2Fixed& out_rect) const {
    auto it = atlas_regions.find(region_name);
    if (it != atlas_regions.end()) {
        out_texture_id = it->second.texture_id;
        out_rect = it->second.rect;
        return true;
    }
    return false;
}

SDL_Texture* TextureServer::get_texture(uint32_t texture_id) const {
    if (texture_id < textures.size()) {
        return textures[texture_id].sdl_texture;
    }
    return nullptr;
}

Vector2Fixed TextureServer::get_texture_size(uint32_t texture_id) const {
    if (texture_id < textures.size()) {
        return Vector2Fixed::from_floats(
            static_cast<float>(textures[texture_id].width),
            static_cast<float>(textures[texture_id].height)
        );
    }
    return Vector2Fixed::zero();
}

} // namespace RetroNode
