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
    Fixed16 rotation;
    Vector2Fixed scale;
};

class RN_API VisualServer {
private:
    static VisualServer* instance;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* virtual_framebuffer = nullptr;
    
    int virtual_width = 256;
    int virtual_height = 224;

    Vector2Fixed camera_offset;
    float current_render_alpha = 1.0f;
    std::vector<DrawCommand> render_queue;
    size_t last_draw_call_count = 0;

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

    float get_current_render_alpha() const { return current_render_alpha; }

    size_t get_draw_call_count() const { return last_draw_call_count; }

    void clear_render_queue() { render_queue.clear(); }
    void submit_draw_sprite(
        const Vector2Fixed& pos,
        const Vector2Fixed& prev_pos,
        const Vector2Fixed& size,
        const Rect2Fixed& src_rect,
        uint32_t texture_id = 0,
        int z_index = 0,
        SDL_Color color = {255, 255, 255, 255},
        Fixed16 rotation = Fixed16(0),
        Vector2Fixed scale = Vector2Fixed::one()
    );
    void draw_line_2d(const Vector2Fixed& p_start, const Vector2Fixed& p_end, SDL_Color p_color);
    void draw_rect_outline_2d(const Rect2Fixed& p_rect, SDL_Color p_color);

    void render_scene(float alpha);
    void present_fullscreen(int window_width, int window_height);
    void render(float alpha);

    SDL_Texture* get_framebuffer_texture() const { return virtual_framebuffer; }
    SDL_Texture* get_editor_framebuffer_texture(int width, int height);
    void render_editor_scene(float alpha, int width, int height, const Vector2Fixed& cam_pan, float cam_zoom);
    SDL_Renderer* get_renderer() const { return renderer; }

    int get_virtual_width() const { return virtual_width; }
    int get_virtual_height() const { return virtual_height; }
private:
    SDL_Texture* editor_framebuffer = nullptr;
    int editor_width = 0;
    int editor_height = 0;
};

} // namespace RetroNode

#endif // RETRONODE_VISUAL_SERVER_H
