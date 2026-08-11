#include "main_menu_bar.h"
#include "project_settings_window.h"
#include "../editor_state.h"
#include "../../servers/texture_server.h"
#include <imgui.h>


namespace RetroNode {

void MainMenuBar::draw() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                EditorState::get()->new_scene();
            }
            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                EditorState::get()->save_current_scene();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                // Exit request handling
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, EditorState::get()->can_undo())) {
                EditorState::get()->undo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, EditorState::get()->can_redo())) {
                EditorState::get()->redo();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete Node", "Del")) {
                Node* sel = EditorState::get()->get_selected_node();
                if (sel && sel->get_parent()) {
                    EditorState::get()->push_undo_snapshot();
                    EditorState::get()->set_selected_instance_id(0);
                    sel->queue_free();
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Project")) {
            if (ImGui::MenuItem("⚙️ Project Settings...", NULL)) {
                ProjectSettingsWindow::is_open = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Reset Camera")) {
                EditorState::get()->get_camera().reset();
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("UI Scale / Font Size")) {
                if (ImGui::MenuItem("75%")) EditorState::get()->set_ui_scale(0.75f);
                if (ImGui::MenuItem("100% (Default)")) EditorState::get()->set_ui_scale(1.0f);
                if (ImGui::MenuItem("125%")) EditorState::get()->set_ui_scale(1.25f);
                if (ImGui::MenuItem("150%")) EditorState::get()->set_ui_scale(1.50f);
                if (ImGui::MenuItem("200%")) EditorState::get()->set_ui_scale(2.0f);
                ImGui::Separator();
                if (ImGui::MenuItem("Zoom UI In", "Ctrl++")) EditorState::get()->set_ui_scale(EditorState::get()->get_ui_scale() + 0.15f);
                if (ImGui::MenuItem("Zoom UI Out", "Ctrl+-")) EditorState::get()->set_ui_scale(EditorState::get()->get_ui_scale() - 0.15f);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        // Play / Stop / Pause Toolbar Controls in Center of Menu Bar
        float bar_width = ImGui::GetWindowWidth();
        ImGui::SameLine(bar_width * 0.5f - 80.0f);

        bool is_playing = EditorState::get()->get_is_play_mode();
        bool is_paused = EditorState::get()->get_is_paused();

        if (!is_playing) {
            if (ImGui::Button(" Play  ")) {
                EditorState::get()->start_play_mode();
            }
        } else {
            if (ImGui::Button(" Stop  ")) {
                EditorState::get()->stop_play_mode();
            }
            ImGui::SameLine();
            if (ImGui::Button(is_paused ? " Resume " : " Pause  ")) {
                EditorState::get()->toggle_pause();
            }
        }

        // Status text and App Branding Logo on far right
        uint32_t logo_tex_id = TextureServer::get()->load_texture("assets/logo_128.png");
        SDL_Texture* logo_tex = TextureServer::get()->get_texture(logo_tex_id);
        if (logo_tex) {
            SDL_SetTextureScaleMode(logo_tex, SDL_SCALEMODE_NEAREST);
            ImGui::SameLine(bar_width - 130.0f);
            ImGui::Image((ImTextureID)(intptr_t)logo_tex, ImVec2(18.0f, 18.0f));
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.3f, 0.75f, 1.0f, 1.0f), "RetroNode");
        } else {
            const std::string& status = EditorState::get()->get_status_message();
            float status_w = ImGui::CalcTextSize(status.c_str()).x;
            ImGui::SameLine(bar_width - status_w - 20.0f);
            ImGui::TextDisabled("%s", status.c_str());
        }

        ImGui::EndMainMenuBar();
    }


    ProjectSettingsWindow::draw();
}

} // namespace RetroNode
