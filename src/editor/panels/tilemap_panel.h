#ifndef RETRONODE_TILEMAP_PANEL_H
#define RETRONODE_TILEMAP_PANEL_H

namespace RetroNode {

class TilemapPanel {
public:
    static int active_tile_index;
    static bool paint_solid;
    static unsigned int active_metadata;
    static bool is_eraser;

    static void draw();
};

} // namespace RetroNode

#endif // RETRONODE_TILEMAP_PANEL_H
