#include "game_view_panel.h"
#include "../editor_state.h"
#include "../../servers/visual_server.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace RetroNode {

void GameViewPanel::draw() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    bool window_open = ImGui::Begin("🎮 Game View", NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();

    if (window_open) {
        bool is_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        EditorState::get()->set_game_view_active(is_focused);

        ImVec2 avail = ImGui::GetContentRegionAvail();
        int v_w = VisualServer::get()->get_virtual_width();
        int v_h = VisualServer::get()->get_virtual_height();

        if (avail.x > 0 && avail.y > 0 && v_w > 0 && v_h > 0) {
            int scale_factor = std::max(1, (int)std::min(avail.x / (float)v_w, (avail.y - 32.0f) / (float)v_h));
            float rendered_w = static_cast<float>(v_w * scale_factor);
            float rendered_h = static_cast<float>(v_h * scale_factor);

            float pad_x = std::floor((avail.x - rendered_w) * 0.5f);
            float pad_y = std::floor(((avail.y - 32.0f) - rendered_h) * 0.5f + 32.0f);

            bool is_playing = EditorState::get()->get_is_play_mode();

            // Header Banner for Gameplay Window
            ImGui::SetCursorPos(ImVec2(10.0f, 6.0f));
            ImGui::BeginGroup();
            if (is_playing) {
                ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "🎮 ACTIVE GAMEPLAY SESSION");
                ImGui::SameLine();
                ImGui::Text(" | Main Scene: %s | FPS: 60", EditorState::get()->get_main_scene_path().c_str());
                ImGui::SameLine(std::max(10.0f, avail.x - 140.0f));
                if (ImGui::Button("⏹️ Stop Game")) {
                    EditorState::get()->stop_play_mode();
                }
            } else {
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "🎮 GAMEPLAY INACTIVE");
                ImGui::SameLine();
                ImGui::Text(" | Press Play (F5) to launch %s", EditorState::get()->get_main_scene_path().c_str());
                ImGui::SameLine(std::max(10.0f, avail.x - 140.0f));
                if (ImGui::Button("▶️ Start Game")) {
                    EditorState::get()->start_play_mode();
                }
            }
            ImGui::EndGroup();

            ImGui::SetCursorPos(ImVec2(pad_x, pad_y));

            SDL_Texture* tex = VisualServer::get()->get_framebuffer_texture();
            if (tex) {
                SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
                ImGui::Image((ImTextureID)(intptr_t)tex, ImVec2(rendered_w, rendered_h));
            } else {
                ImGui::Dummy(ImVec2(rendered_w, rendered_h));
            }
        }
    } else {
        EditorState::get()->set_game_view_active(false);
    }

    ImGui::End();
}

} // namespace RetroNode
