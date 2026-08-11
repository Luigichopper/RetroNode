#include "resource_manager.h"
#include <iostream>
#include <filesystem>
#include <stb_image.h>

#include "../servers/texture_server.h"

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
    TextureServer::get()->init(p_renderer);
}

SDL_Texture* ResourceManager::load_texture(const std::string& p_path) {
    uint32_t id = TextureServer::get()->load_texture(p_path);
    return TextureServer::get()->get_texture(id);
}

bool ResourceManager::has_texture(const std::string& p_path) const {
    uint32_t id = TextureServer::get()->load_texture(p_path);
    return id != 0;
}

void ResourceManager::clear_cache() {
    texture_cache.clear();
}

} // namespace RetroNode
