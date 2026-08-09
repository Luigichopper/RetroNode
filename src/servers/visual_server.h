#ifndef RETRONODE_VISUAL_SERVER_H
#define RETRONODE_VISUAL_SERVER_H

#include <SDL3/SDL.h>
#include <vector>
#include <cmath>
#include "../core/math/vector2.h"
#include "../core/math/rect2.h"
#include "../core/object/class_db.h"

namespace RetroNode {

struct DrawCommand {
    Vector2Fixed world_position;
    Vector2Fixed previous_position;
    Vector2Fixed size;
    Rect2Fixed src_rect;
    uint32_t texture_id;
    int z_index;
    SDL_Color color;
};

class RN_API VisualServer {
private:
    static VisualServer* instance;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* virtual_framebuffer = nullptr;
    
    int virtual_width = 256;
    int virtual_height = 224;

    Vector2Fixed camera_offset;
    std::vector<DrawCommand> render_queue;

public:
    VisualServer();
    ~VisualServer();

    static VisualServer* get() {
        if (!instance) {
            instance = new VisualServer();
        }
        return instance;
    }

    void init(SDL_Renderer* p_renderer, int v_width = 256, int v_height = 224);
    void shutdown();

    void set_camera_offset(const Vector2Fixed& offset) { camera_offset = offset; }
    Vector2Fixed get_camera_offset() const { return camera_offset; }

    void clear_render_queue() { render_queue.clear(); }
    void submit_draw_sprite(
        const Vector2Fixed& pos,
        const Vector2Fixed& prev_pos,
        const Vector2Fixed& size,
        const Rect2Fixed& src_rect,
        uint32_t texture_id = 0,
        int z_index = 0,
        SDL_Color color = {255, 255, 255, 255}
    );
    
    void render(float alpha);

    int get_virtual_width() const { return virtual_width; }
    int get_virtual_height() const { return virtual_height; }
};

} // namespace RetroNode

#endif // RETRONODE_VISUAL_SERVER_H
