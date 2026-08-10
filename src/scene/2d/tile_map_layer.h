#ifndef RETRONODE_TILE_MAP_LAYER_H
#define RETRONODE_TILE_MAP_LAYER_H

#include "node_2d.h"
#include <vector>
#include <string>
#include <SDL3/SDL.h>

namespace RetroNode {

class RN_API TileMapLayer : public Node2D {
    RN_CLASS(TileMapLayer, Node2D)

public:
    int columns = 16;
    int rows = 14;
    int tile_size = 16;
    uint32_t tileset_texture_id = 0;
    std::string tileset_path;

    std::vector<int> tile_data;
    std::vector<bool> collision_data;
    int z_index = -10;
    SDL_Color modulate = { 255, 255, 255, 255 };

    TileMapLayer();
    virtual ~TileMapLayer() = default;

    virtual void get_property_list(std::vector<PropertyInfo>& out_list) const override;
    virtual Variant get(const StringName& p_name) const override;
    virtual bool set(const StringName& p_name, const Variant& p_value) override;

    void set_tileset_path(const std::string& path);
    void setup_map(int p_cols, int p_rows, int p_tile_sz, const std::vector<int>& p_tiles, const std::vector<bool>& p_collisions);

    virtual void _ready() override;
    virtual void _process(float delta) override;
};

} // namespace RetroNode

#endif // RETRONODE_TILE_MAP_LAYER_H
