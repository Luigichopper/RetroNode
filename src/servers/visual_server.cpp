#include "visual_server.h"
#include "texture_server.h"
#ifdef RN_BUILD_EDITOR
#include "../editor/editor_state.h"
#endif
#include <algorithm>
#include <iostream>
#include <cmath>

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

    render_queue.reserve(256);
    TextureServer::get()->init(renderer);

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
    TextureServer::get()->shutdown();
    if (virtual_framebuffer) {
        SDL_DestroyTexture(virtual_framebuffer);
        virtual_framebuffer = nullptr;
    }
}

void VisualServer::submit_draw_sprite(
    const Vector2Fixed& pos,
    const Vector2Fixed& prev_pos,
    const Vector2Fixed& size,
    const Rect2Fixed& src_rect,
    uint32_t texture_id,
    int z_index,
    SDL_Color color,
    Fixed16 rotation,
    Vector2Fixed scale
) {
    render_queue.push_back({pos, prev_pos, size, src_rect, texture_id, z_index, color, rotation, scale});
}

void VisualServer::draw_line_2d(const Vector2Fixed& p_start, const Vector2Fixed& p_end, SDL_Color p_color) {
    if (!renderer) return;

    SDL_SetRenderTarget(renderer, virtual_framebuffer);
    SDL_SetRenderDrawColor(renderer, p_color.r, p_color.g, p_color.b, p_color.a);
    SDL_RenderLine(renderer, p_start.x.to_float(), p_start.y.to_float(), p_end.x.to_float(), p_end.y.to_float());
}



void VisualServer::draw_rect_outline_2d(const Rect2Fixed& p_rect, SDL_Color p_color) {
    if (!renderer) return;

    Vector2Fixed screen_pos = p_rect.position - camera_offset;

    SDL_FRect rect = {
        screen_pos.x.to_float(),
        screen_pos.y.to_float(),
        p_rect.size.x.to_float(),
        p_rect.size.y.to_float()
    };

    SDL_SetRenderDrawColor(renderer, p_color.r, p_color.g, p_color.b, p_color.a);
    SDL_RenderRect(renderer, &rect);
}

void VisualServer::render_scene(float alpha) {
    if (!renderer || !virtual_framebuffer) return;

    SDL_SetRenderTarget(renderer, virtual_framebuffer);
    SDL_SetRenderDrawColor(renderer, 20, 20, 24, 255);
    SDL_RenderClear(renderer);

    std::sort(render_queue.begin(), render_queue.end(), [](const DrawCommand& a, const DrawCommand& b) {
        return a.z_index < b.z_index;
    });

    Vector2Fixed active_cam_offset = camera_offset;
    float active_cam_zoom = 1.0f;

#ifdef RN_BUILD_EDITOR
    if (!EditorState::get()->get_is_play_mode() || !EditorState::get()->is_game_view_active()) {
        active_cam_offset = EditorState::get()->get_camera().pan;
        active_cam_zoom = EditorState::get()->get_camera().zoom;
    }
#endif

    // Draw all submitted quads with frame interpolation & sub-pixel truncation (floor) for retro grid snapping
    for (const auto& cmd : render_queue) {
        // Interpolate world position between previous frame position and current position
        Vector2Fixed interpolated_world_pos = cmd.world_position;
#ifdef RN_BUILD_EDITOR
        bool is_play = EditorState::get()->get_is_play_mode();
#else
        bool is_play = true;
#endif
        if (is_play && alpha < 0.999f && cmd.world_position != cmd.previous_position) {
            interpolated_world_pos.x = Fixed16::from_float(cmd.previous_position.x.to_float() + (cmd.world_position.x.to_float() - cmd.previous_position.x.to_float()) * alpha);
            interpolated_world_pos.y = Fixed16::from_float(cmd.previous_position.y.to_float() + (cmd.world_position.y.to_float() - cmd.previous_position.y.to_float()) * alpha);
        }

        Vector2Fixed screen_pos = (interpolated_world_pos - active_cam_offset) * Fixed16::from_float(active_cam_zoom);

        // Sub-pixel truncation (floor) to guarantee alignment with retro pixel grid
        float draw_x = std::floor(screen_pos.x.to_float());
        float draw_y = std::floor(screen_pos.y.to_float());
        float draw_w = cmd.size.x.to_float() * cmd.scale.x.to_float() * active_cam_zoom;
        float draw_h = cmd.size.y.to_float() * cmd.scale.y.to_float() * active_cam_zoom;

        SDL_FRect dst_rect = { draw_x, draw_y, draw_w, draw_h };
        double rot_deg = static_cast<double>(cmd.rotation.to_float());

        SDL_Texture* tex = TextureServer::get()->get_texture(cmd.texture_id);

        if (tex) {
            SDL_SetTextureColorMod(tex, cmd.color.r, cmd.color.g, cmd.color.b);
            SDL_SetTextureAlphaMod(tex, cmd.color.a);
            SDL_FRect src_frect = {
                cmd.src_rect.position.x.to_float(),
                cmd.src_rect.position.y.to_float(),
                cmd.src_rect.size.x.to_float(),
                cmd.src_rect.size.y.to_float()
            };
            if (rot_deg != 0.0) {
                SDL_RenderTextureRotated(renderer, tex, &src_frect, &dst_rect, rot_deg, NULL, SDL_FLIP_NONE);
            } else {
                SDL_RenderTexture(renderer, tex, &src_frect, &dst_rect);
            }
        } else {
            // Fallback procedural retro quad rendering if texture is invalid/null
            SDL_SetRenderDrawColor(renderer, cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a);
            SDL_RenderFillRect(renderer, &dst_rect);
        }
    }

    last_draw_call_count = render_queue.size();
    render_queue.clear();
}

void VisualServer::present_fullscreen(int win_w, int win_h) {
    if (!renderer || !virtual_framebuffer) return;

    // 2. Blit Virtual Framebuffer to Window Screen with Integer Scaling
    SDL_SetRenderTarget(renderer, NULL);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (win_w <= 0 || win_h <= 0) {
        SDL_GetRenderOutputSize(renderer, &win_w, &win_h);
    }

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

void VisualServer::render(float alpha) {
    render_scene(alpha);
    present_fullscreen(0, 0);
}

} // namespace RetroNode
