#include "visual_server.h"
#include <algorithm>
#include <iostream>

namespace RetroNode {

VisualServer* VisualServer::instance = nullptr;

VisualServer::VisualServer() : camera_offset(Vector2Fixed::zero()) {}

VisualServer::~VisualServer() {
    shutdown();
}

void VisualServer::init(SDL_Renderer* p_renderer, int v_width, int v_height) {
    renderer = p_renderer;
    virtual_width = v_width;
    virtual_height = v_height;

    if (virtual_framebuffer) {
        SDL_DestroyTexture(virtual_framebuffer);
        virtual_framebuffer = nullptr;
    }

    if (renderer) {
        virtual_framebuffer = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET,
            virtual_width,
            virtual_height
        );
        
        if (virtual_framebuffer) {
            SDL_SetTextureScaleMode(virtual_framebuffer, SDL_SCALEMODE_NEAREST);
        } else {
            std::cerr << "[VisualServer] Error creating target texture: " << SDL_GetError() << std::endl;
        }
    }
}

void VisualServer::shutdown() {
    if (virtual_framebuffer) {
        SDL_DestroyTexture(virtual_framebuffer);
        virtual_framebuffer = nullptr;
    }
}

void VisualServer::submit_draw_sprite(const Vector2Fixed& pos, const Vector2Fixed& size, const Rect2Fixed& src_rect, int z_index, SDL_Color color) {
    render_queue.push_back({pos, size, src_rect, z_index, color});
}

void VisualServer::render(float alpha) {
    (void)alpha;

    if (!renderer || !virtual_framebuffer) return;

    // 1. Render to Virtual Framebuffer Target
    SDL_SetRenderTarget(renderer, virtual_framebuffer);
    SDL_SetRenderDrawColor(renderer, 24, 20, 36, 255); // Retro background color
    SDL_RenderClear(renderer);

    // Sort draw commands by z-index
    std::sort(render_queue.begin(), render_queue.end(), [](const DrawCommand& a, const DrawCommand& b) {
        return a.z_index < b.z_index;
    });

    // Draw all submitted quads with sub-pixel truncation (floor) for retro grid snapping
    for (const auto& cmd : render_queue) {
        Vector2Fixed screen_pos = cmd.world_position - camera_offset;
        
        // Sub-pixel truncation (floor) to guarantee alignment with retro pixel grid
        float draw_x = std::floor(screen_pos.x.to_float());
        float draw_y = std::floor(screen_pos.y.to_float());
        float draw_w = cmd.size.x.to_float();
        float draw_h = cmd.size.y.to_float();

        SDL_FRect dst_rect = { draw_x, draw_y, draw_w, draw_h };

        // Fallback procedural retro quad rendering if texture src_rect is zero
        SDL_SetRenderDrawColor(renderer, cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a);
        SDL_RenderFillRect(renderer, &dst_rect);
    }

    render_queue.clear();

    // 2. Blit Virtual Framebuffer to Window Screen with Integer Scaling
    SDL_SetRenderTarget(renderer, NULL);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    int win_w = 0, win_h = 0;
    SDL_GetRenderOutputSize(renderer, &win_w, &win_h);

    // Calculate integer scale factor
    int scale_x = win_w / virtual_width;
    int scale_y = win_h / virtual_height;
    int scale = (scale_x < scale_y) ? scale_x : scale_y;
    if (scale < 1) scale = 1;

    int dst_w = virtual_width * scale;
    int dst_h = virtual_height * scale;
    int dst_x = (win_w - dst_w) / 2;
    int dst_y = (win_h - dst_h) / 2;

    SDL_FRect window_dst = {
        static_cast<float>(dst_x),
        static_cast<float>(dst_y),
        static_cast<float>(dst_w),
        static_cast<float>(dst_h)
    };

    SDL_RenderTexture(renderer, virtual_framebuffer, NULL, &window_dst);
    SDL_RenderPresent(renderer);
}

} // namespace RetroNode
