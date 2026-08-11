#include "tile_map_layer.h"
#include "../../servers/visual_server.h"
#include "../../servers/texture_server.h"
#include "../../servers/physics_server.h"

namespace RetroNode {

TileMapLayer::TileMapLayer() {
    name = "TileMapLayer";
}

void TileMapLayer::get_property_list(std::vector<PropertyInfo>& out_list) const {
    Node2D::get_property_list(out_list);
    out_list.push_back({ StringName("tileset"), VariantType::STRING, PropertyHint::FILE_PATH, "*.png" });
    out_list.push_back({ StringName("z_index"), VariantType::INT });
    out_list.push_back({ StringName("modulate"), VariantType::COLOR });
}

Variant TileMapLayer::get(const StringName& p_name) const {
    static const StringName s_tileset("tileset");
    static const StringName s_tileset_path("tileset_path");
    static const StringName s_z_index("z_index");
    static const StringName s_modulate("modulate");

    if (p_name == s_tileset || p_name == s_tileset_path) return Variant(tileset_path);
    if (p_name == s_z_index) return Variant((int64_t)z_index);
    if (p_name == s_modulate) return Variant(modulate);
    return Node2D::get(p_name);
}

bool TileMapLayer::set(const StringName& p_name, const Variant& p_value) {
    static const StringName s_tileset("tileset");
    static const StringName s_tileset_path("tileset_path");
    static const StringName s_z_index("z_index");
    static const StringName s_modulate("modulate");

    if (p_name == s_tileset || p_name == s_tileset_path) {
        set_tileset_path(p_value.as_string());
        return true;
    }
    if (p_name == s_z_index) {
        z_index = static_cast<int>(p_value.as_int());
        return true;
    }
    if (p_name == s_modulate) {
        modulate = p_value.as_color();
        return true;
    }
    return Node2D::set(p_name, p_value);
}

void TileMapLayer::set_tileset_path(const std::string& path) {
    tileset_path = path;
    if (!tileset_path.empty()) {
        tileset_texture_id = TextureServer::get()->load_texture(tileset_path);
        cached_texture_id = 0; // Trigger recalculation in _process
    }
}

void TileMapLayer::setup_map(int p_cols, int p_rows, int p_tile_sz, const std::vector<int>& p_tiles, const std::vector<bool>& p_collisions) {
    columns = p_cols;
    rows = p_rows;
    tile_size = p_tile_sz;
    tile_data = p_tiles;
    collision_data = p_collisions;
    cell_metadata.resize(columns * rows, 0);
}

int TileMapLayer::get_cell(int col, int row) const {
    if (col < 0 || col >= columns || row < 0 || row >= rows) return -1;
    size_t idx = row * columns + col;
    return (idx < tile_data.size()) ? tile_data[idx] : -1;
}

bool TileMapLayer::is_cell_solid(int col, int row) const {
    if (col < 0 || col >= columns || row < 0 || row >= rows) return false;
    size_t idx = row * columns + col;
    return (idx < collision_data.size()) ? collision_data[idx] : false;
}

uint32_t TileMapLayer::get_cell_metadata(int col, int row) const {
    if (col < 0 || col >= columns || row < 0 || row >= rows) return 0;
    size_t idx = row * columns + col;
    return (idx < cell_metadata.size()) ? cell_metadata[idx] : 0;
}

void TileMapLayer::set_cell(int col, int row, int tile_idx, bool solid, uint32_t metadata) {
    if (col < 0 || col >= columns || row < 0 || row >= rows) return;
    size_t idx = row * columns + col;
    if (idx >= tile_data.size()) {
        tile_data.resize(columns * rows, -1);
        collision_data.resize(columns * rows, false);
        cell_metadata.resize(columns * rows, 0);
    }
    tile_data[idx] = tile_idx;
    collision_data[idx] = solid;
    cell_metadata[idx] = metadata;
}

void TileMapLayer::set_cell_metadata(int col, int row, uint32_t metadata) {
    if (col < 0 || col >= columns || row < 0 || row >= rows) return;
    size_t idx = row * columns + col;
    if (idx >= cell_metadata.size()) {
        cell_metadata.resize(columns * rows, 0);
    }
    cell_metadata[idx] = metadata;
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
                uint64_t tile_box_id = 0x8000000000000000ULL | (get_instance_id() << 32) | static_cast<uint64_t>(idx);
                PhysicsServer2D::get()->add_static_box(tile_box_id, bounds);
            }
        }
    }
}

void TileMapLayer::_process(float delta) {
    (void)delta;
    if (!is_visible_in_tree()) return;

    if (tileset_texture_id == 0 && !tileset_path.empty()) {
        tileset_texture_id = TextureServer::get()->load_texture(tileset_path);
    }

    if (tileset_texture_id != cached_texture_id) {
        cached_texture_id = tileset_texture_id;
        cached_tex_size = TextureServer::get()->get_texture_size(tileset_texture_id);
        cached_tileset_cols = (cached_tex_size.x > Fixed16(0)) ? (cached_tex_size.x.to_int() / tile_size) : 1;
        if (cached_tileset_cols < 1) cached_tileset_cols = 1;
    }

    Vector2Fixed global_pos = get_global_position();
    Vector2Fixed global_prev_pos = get_global_previous_position();

    Vector2Fixed cam_offset = VisualServer::get()->get_camera_offset();
    Fixed16 cam_min_x = cam_offset.x - Fixed16(tile_size);
    Fixed16 cam_max_x = cam_offset.x + Fixed16(256 + tile_size);
    Fixed16 cam_min_y = cam_offset.y - Fixed16(tile_size);
    Fixed16 cam_max_y = cam_offset.y + Fixed16(224 + tile_size);

    bool is_editor = false;
#ifdef RN_BUILD_EDITOR
    is_editor = true;
#endif

    int tileset_cols = cached_tileset_cols;

    int min_c = 0;
    int max_c = columns - 1;
    int min_r = 0;
    int max_r = rows - 1;

    if (!is_editor) {
        float rel_min_x = (cam_min_x - global_pos.x).to_float();
        float rel_max_x = (cam_max_x - global_pos.x).to_float();
        float rel_min_y = (cam_min_y - global_pos.y).to_float();
        float rel_max_y = (cam_max_y - global_pos.y).to_float();

        min_c = std::max(0, static_cast<int>(std::floor(rel_min_x / tile_size)));
        max_c = std::min(columns - 1, static_cast<int>(std::ceil(rel_max_x / tile_size)));
        min_r = std::max(0, static_cast<int>(std::floor(rel_min_y / tile_size)));
        max_r = std::min(rows - 1, static_cast<int>(std::ceil(rel_max_y / tile_size)));
    }

    for (int r = min_r; r <= max_r; ++r) {
        for (int c = min_c; c <= max_c; ++c) {
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
