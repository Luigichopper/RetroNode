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

private:
    mutable uint32_t cached_texture_id = 0;
    mutable int cached_tileset_cols = 1;
    mutable Vector2Fixed cached_tex_size = Vector2Fixed::zero();

public:

    TileMapLayer();
    virtual ~TileMapLayer() = default;

    virtual void get_property_list(std::vector<PropertyInfo>& out_list) const override;
    virtual Variant get(const StringName& p_name) const override;
    virtual bool set(const StringName& p_name, const Variant& p_value) override;

    void set_tileset_path(const std::string& path);
    const std::string& get_tileset_path() const { return tileset_path; }

    void set_z_index(int z) { z_index = z; }
    int get_z_index() const { return z_index; }

    void set_modulate(const SDL_Color& m) { modulate = m; }
    SDL_Color get_modulate() const { return modulate; }

    void set_columns(int c) { columns = c; }
    int get_columns() const { return columns; }

    void set_rows(int r) { rows = r; }
    int get_rows() const { return rows; }

    void set_tile_size(int sz) { tile_size = sz; }
    int get_tile_size() const { return tile_size; }

    void setup_map(int p_cols, int p_rows, int p_tile_sz, const std::vector<int>& p_tiles, const std::vector<bool>& p_collisions);

    virtual void _ready() override;
    virtual void _process(float delta) override;
};

} // namespace RetroNode

#endif // RETRONODE_TILE_MAP_LAYER_H
