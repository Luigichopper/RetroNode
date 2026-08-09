#include "tile_map_layer.h"
#include "../../servers/visual_server.h"
#include "../../servers/texture_server.h"
#include "../../servers/physics_server.h"

namespace RetroNode {

TileMapLayer::TileMapLayer() {
    name = "TileMapLayer";
}

void TileMapLayer::set_tileset_path(const std::string& path) {
    tileset_path = path;
    if (!tileset_path.empty()) {
        tileset_texture_id = TextureServer::get()->load_texture(tileset_path);
    }
}

void TileMapLayer::setup_map(int p_cols, int p_rows, int p_tile_sz, const std::vector<int>& p_tiles, const std::vector<bool>& p_collisions) {
    columns = p_cols;
    rows = p_rows;
    tile_size = p_tile_sz;
    tile_data = p_tiles;
    collision_data = p_collisions;
}

void TileMapLayer::_ready() {
    // Register solid tiles with PhysicsServer2D spatial hash grid
    Vector2Fixed global_pos = get_global_position();

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < columns; ++c) {
            size_t idx = r * columns + c;
            if (idx < collision_data.size() && collision_data[idx]) {
                Fixed16 tx = global_pos.x + Fixed16::from_int(c * tile_size);
                Fixed16 ty = global_pos.y + Fixed16::from_int(r * tile_size);
                Rect2Fixed bounds = Rect2Fixed::from_floats(
                    tx.to_float(),
                    ty.to_float(),
                    static_cast<float>(tile_size),
                    static_cast<float>(tile_size)
                );
                PhysicsServer2D::get()->add_static_box(get_instance_id() + idx, bounds);
            }
        }
    }
}

void TileMapLayer::_process(float delta) {
    (void)delta;

    Vector2Fixed global_pos = get_global_position();
    Vector2Fixed global_prev_pos = get_global_previous_position();
    Vector2Fixed tex_size = TextureServer::get()->get_texture_size(tileset_texture_id);

    int tileset_cols = (tex_size.x > Fixed16(0)) ? (tex_size.x.to_int() / tile_size) : 1;
    if (tileset_cols < 1) tileset_cols = 1;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < columns; ++c) {
            size_t idx = r * columns + c;
            if (idx >= tile_data.size()) continue;

            int tile_idx = tile_data[idx];
            if (tile_idx < 0) continue; // Skip empty air tiles

            int src_x = (tile_idx % tileset_cols) * tile_size;
            int src_y = (tile_idx / tileset_cols) * tile_size;

            Fixed16 world_x = global_pos.x + Fixed16::from_int(c * tile_size);
            Fixed16 world_y = global_pos.y + Fixed16::from_int(r * tile_size);

            Fixed16 prev_world_x = global_prev_pos.x + Fixed16::from_int(c * tile_size);
            Fixed16 prev_world_y = global_prev_pos.y + Fixed16::from_int(r * tile_size);

            Vector2Fixed tile_world_pos(world_x, world_y);
            Vector2Fixed tile_prev_pos(prev_world_x, prev_world_y);
            Vector2Fixed tile_sz = Vector2Fixed::from_floats(static_cast<float>(tile_size), static_cast<float>(tile_size));
            Rect2Fixed src_rect = Rect2Fixed::from_floats(
                static_cast<float>(src_x),
                static_cast<float>(src_y),
                static_cast<float>(tile_size),
                static_cast<float>(tile_size)
            );

            VisualServer::get()->submit_draw_sprite(
                tile_world_pos,
                tile_prev_pos,
                tile_sz,
                src_rect,
                tileset_texture_id,
                z_index,
                modulate
            );
        }
    }
}

} // namespace RetroNode
