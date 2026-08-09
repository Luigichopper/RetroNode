#include "texture_server.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>
#include <filesystem>

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

    std::vector<std::string> candidates = {
        project_dir + "/" + resolved_path,
        "./" + resolved_path,
        "./MyRPG/" + resolved_path,
        "../MyRPG/" + resolved_path,
        "../../MyRPG/" + resolved_path,
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
        std::cerr << "[TextureServer] Warning: Could not locate image resource '" << filepath << "'" << std::endl;
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
