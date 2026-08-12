#ifndef RETRONODE_SPRITE_FRAMES_PANEL_H
#define RETRONODE_SPRITE_FRAMES_PANEL_H

#include "../../scene/2d/animated_sprite_2d.h"
#include <string>
#include <vector>

namespace RetroNode {

class SpriteFramesPanel {
public:
    static bool open;
    static std::string active_anim_name;
    static int active_frame_index;
    static bool preview_playing;
    static float preview_timer;
    static int preview_frame;

    // Sprite Sheet Import Modal State
    static bool show_sheet_modal;
    static std::string sheet_texture_path;
    static int slice_mode; // 0 = Count (cols x rows), 1 = Size (w x h)
    static int sheet_cols;
    static int sheet_rows;
    static int frame_width;
    static int frame_height;
    static int separation_x;
    static int separation_y;
    static int offset_x;
    static int offset_y;
    static std::vector<bool> cell_selection;

    static void draw();
    static void open_sheet_modal(const std::string& default_path = "");

private:
    static void draw_animations_list(AnimatedSprite2D* node, SpriteFrames* sf);
    static void draw_frames_view(AnimatedSprite2D* node, SpriteFrames* sf);
    static void draw_sprite_sheet_modal(AnimatedSprite2D* node, SpriteFrames* sf);
};

} // namespace RetroNode

#endif // RETRONODE_SPRITE_FRAMES_PANEL_H
