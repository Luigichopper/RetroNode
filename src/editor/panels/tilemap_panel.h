#ifndef RETRONODE_TILEMAP_PANEL_H
#define RETRONODE_TILEMAP_PANEL_H

#include "../../scene/2d/tile_map_layer.h"

namespace RetroNode {

enum class TileTool {
    PENCIL = 0,
    ERASER = 1,
    BUCKET_FILL = 2,
    RECT_FILL = 3,
    COLLISION_TOGGLE = 4
};

class TilemapPanel {
public:
    static TileTool current_tool;
    static int active_tile_index;
    static bool paint_solid;
    static unsigned int active_metadata;
    static bool show_grid_overlay;
    static bool show_collision_overlay;

    // Rect fill drag state
    static bool is_rect_drag;
    static int rect_start_col;
    static int rect_start_row;

    static void draw();
    static void perform_flood_fill(TileMapLayer* tilemap, int start_col, int start_row, int fill_tile, bool solid, uint32_t metadata);
    static void perform_rect_fill(TileMapLayer* tilemap, int col1, int row1, int col2, int row2, int fill_tile, bool solid, uint32_t metadata);
};

} // namespace RetroNode

#endif // RETRONODE_TILEMAP_PANEL_H
