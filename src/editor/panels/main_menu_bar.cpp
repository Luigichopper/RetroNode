#include "main_menu_bar.h"
#include "../editor_state.h"
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
            if (ImGui::MenuItem("Delete Node", "Del")) {
                Node* sel = EditorState::get()->get_selected_node();
                if (sel && sel->get_parent()) {
                    EditorState::get()->set_selected_instance_id(0);
                    sel->queue_free();
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Reset Camera")) {
                EditorState::get()->get_camera().reset();
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

        // Status text on far right
        const std::string& status = EditorState::get()->get_status_message();
        float status_w = ImGui::CalcTextSize(status.c_str()).x;
        ImGui::SameLine(bar_width - status_w - 20.0f);
        ImGui::TextDisabled("%s", status.c_str());

        ImGui::EndMainMenuBar();
    }
}

} // namespace RetroNode
