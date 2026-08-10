#ifndef RETRONODE_RESOURCE_MANAGER_H
#define RETRONODE_RESOURCE_MANAGER_H

#include "object/object.h"
#include <string>
#include <unordered_map>
#include <SDL3/SDL.h>

namespace RetroNode {

class RN_API ResourceManager {
private:
    static ResourceManager* instance;
    SDL_Renderer* renderer_ref = nullptr;
    std::unordered_map<std::string, SDL_Texture*> texture_cache;

public:
    ResourceManager() = default;
    ~ResourceManager();

    static ResourceManager& get_singleton();
    void init(SDL_Renderer* p_renderer);

    SDL_Texture* load_texture(const std::string& p_path);
    bool has_texture(const std::string& p_path) const;

    void clear_cache();
};

} // namespace RetroNode

#endif // RETRONODE_RESOURCE_MANAGER_H
