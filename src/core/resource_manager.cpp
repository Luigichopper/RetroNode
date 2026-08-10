#include "resource_manager.h"
#include <iostream>
#include <filesystem>
#include <stb_image.h>

namespace fs = std::filesystem;

namespace RetroNode {

ResourceManager* ResourceManager::instance = nullptr;

ResourceManager& ResourceManager::get_singleton() {
    if (!instance) {
        instance = new ResourceManager();
    }
    return *instance;
}

ResourceManager::~ResourceManager() {
    clear_cache();
}

void ResourceManager::init(SDL_Renderer* p_renderer) {
    renderer_ref = p_renderer;
}

SDL_Texture* ResourceManager::load_texture(const std::string& p_path) {
    auto it = texture_cache.find(p_path);
    if (it != texture_cache.end()) {
        return it->second;
    }

    if (!renderer_ref) {
        return nullptr;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = stbi_load(p_path.c_str(), &width, &height, &channels, 4);
    if (!data) {
        return nullptr;
    }

    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        width,
        height,
        SDL_PIXELFORMAT_RGBA32,
        data,
        width * 4
    );

    if (!surface) {
        stbi_image_free(data);
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_ref, surface);
    SDL_DestroySurface(surface);
    stbi_image_free(data);

    if (texture) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        texture_cache[p_path] = texture;
    }

    return texture;
}

bool ResourceManager::has_texture(const std::string& p_path) const {
    return texture_cache.find(p_path) != texture_cache.end();
}

void ResourceManager::clear_cache() {
    for (auto& [path, texture] : texture_cache) {
        if (texture) {
            SDL_DestroyTexture(texture);
        }
    }
    texture_cache.clear();
}

} // namespace RetroNode
