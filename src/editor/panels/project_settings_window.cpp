#include "project_settings_window.h"
#include "../editor_state.h"
#include "../../servers/input.h"
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <string>

namespace RetroNode {

bool ProjectSettingsWindow::is_open = false;
static char new_action_buf[128] = "";
static std::string capturing_action = "";
static bool is_capturing_key = false;

void ProjectSettingsWindow::draw() {
    if (!is_open) return;

    ImGui::SetNextWindowSize(ImVec2(650.0f, 480.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("⚙️ Project Settings", &is_open, ImGuiWindowFlags_NoDocking)) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("ProjectSettingsTabs")) {
        // ----------------------------------------------------
        // Tab 1: Godot-Style Input Map Configuration
        // ----------------------------------------------------
        if (ImGui::BeginTabItem("🎮 Input Map")) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Add Action:");
            ImGui::SameLine();
            ImGui::PushItemWidth(240.0f);
            ImGui::InputText("##NewActionInput", new_action_buf, sizeof(new_action_buf));
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button("➕ Add Action") && new_action_buf[0] != '\0') {
                Input::get()->add_action(StringName(new_action_buf));
                new_action_buf[0] = '\0';
            }

            ImGui::Separator();
            ImGui::Spacing();

            // Table of actions
            if (ImGui::BeginTable("InputMapTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Action Name", ImGuiTableColumnFlags_WidthFixed, 180.0f);
                ImGui::TableSetupColumn("Bound Keys / Events", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableHeadersRow();

                auto action_list = Input::get()->get_action_list();
                std::string action_to_delete = "";

                for (const auto& act : action_list) {
                    std::string act_name = act.as_string();
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(act_name.c_str());

                    ImGui::TableSetColumnIndex(1);
                    auto keys = Input::get()->get_action_keys(act);

                    if (keys.empty()) {
                        ImGui::TextDisabled("<No keys bound>");
                    } else {
                        for (size_t i = 0; i < keys.size(); ++i) {
                            SDL_Keycode k = keys[i];
                            const char* k_name = SDL_GetKeyName(k);
                            std::string key_str = (k_name && k_name[0] != '\0') ? k_name : std::to_string((int)k);

                            std::string btn_id = "[ " + key_str + " ]##" + act_name + "_" + std::to_string(i);
                            if (ImGui::SmallButton(btn_id.c_str())) {
                                Input::get()->unbind_action(k, act);
                            }
                            if (ImGui::IsItemHovered()) {
                                ImGui::SetTooltip("Click to unbind key '%s'", key_str.c_str());
                            }
                            ImGui::SameLine();
                        }
                    }

                    ImGui::TableSetColumnIndex(2);
                    std::string add_btn_id = "➕ Bind Key##" + act_name;
                    if (ImGui::Button(add_btn_id.c_str())) {
                        capturing_action = act_name;
                        is_capturing_key = true;
                        ImGui::OpenPopup("KeyCapturePopup");
                    }
                    ImGui::SameLine();
                    std::string del_btn_id = "🗑️##" + act_name;
                    if (ImGui::Button(del_btn_id.c_str())) {
                        action_to_delete = act_name;
                    }
                }

                if (!action_to_delete.empty()) {
                    Input::get()->remove_action(StringName(action_to_delete));
                }

                ImGui::EndTable();
            }

            // Key capture modal popup
            if (ImGui::BeginPopupModal("KeyCapturePopup", &is_capturing_key, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Press any key to bind to action '%s'...", capturing_action.c_str());
                ImGui::Separator();
                ImGui::Spacing();

                ImGuiIO& io = ImGui::GetIO();
                for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; ++key) {
                    if (ImGui::IsKeyPressed((ImGuiKey)key)) {
                        // Map ImGui key to SDL keycode
                        SDL_Keycode sdl_key = SDLK_UNKNOWN;
                        if (key >= ImGuiKey_A && key <= ImGuiKey_Z) sdl_key = SDLK_A + (key - ImGuiKey_A);
                        else if (key >= ImGuiKey_0 && key <= ImGuiKey_9) sdl_key = SDLK_0 + (key - ImGuiKey_0);
                        else if (key == ImGuiKey_Space) sdl_key = SDLK_SPACE;
                        else if (key == ImGuiKey_Enter || key == ImGuiKey_KeypadEnter) sdl_key = SDLK_RETURN;
                        else if (key == ImGuiKey_LeftShift || key == ImGuiKey_RightShift) sdl_key = SDLK_LSHIFT;
                        else if (key == ImGuiKey_LeftCtrl || key == ImGuiKey_RightCtrl) sdl_key = SDLK_LCTRL;
                        else if (key == ImGuiKey_LeftArrow) sdl_key = SDLK_LEFT;
                        else if (key == ImGuiKey_RightArrow) sdl_key = SDLK_RIGHT;
                        else if (key == ImGuiKey_UpArrow) sdl_key = SDLK_UP;
                        else if (key == ImGuiKey_DownArrow) sdl_key = SDLK_DOWN;

                        if (sdl_key != SDLK_UNKNOWN) {
                            Input::get()->bind_action(sdl_key, StringName(capturing_action));
                            is_capturing_key = false;
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }

                if (ImGui::Button("Cancel")) {
                    is_capturing_key = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::EndTabItem();
        }

        // ----------------------------------------------------
        // Tab 2: General Project Config
        // ----------------------------------------------------
        if (ImGui::BeginTabItem("📁 General")) {
            ImGui::Spacing();
            std::string main_scene = EditorState::get()->get_main_scene_path();
            char main_scene_buf[256];
            strncpy(main_scene_buf, main_scene.c_str(), sizeof(main_scene_buf));

            ImGui::Text("Main Scene Path:");
            if (ImGui::InputText("##MainSceneInput", main_scene_buf, sizeof(main_scene_buf))) {
                EditorState::get()->set_main_scene_path(main_scene_buf);
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::Separator();
    ImGui::Spacing();

    // Footer Save Button
    if (ImGui::Button("💾 Save Project Settings", ImVec2(180.0f, 32.0f))) {
        std::string proj_dir = EditorState::get()->get_project_dir();
        std::string proj_file = proj_dir + "/project.rnode";

        nlohmann::json root_json = nlohmann::json::object();

        std::ifstream in(proj_file);
        if (in.is_open()) {
            try { in >> root_json; } catch (...) {}
            in.close();
        }

        root_json["main_scene"] = EditorState::get()->get_main_scene_path();
        root_json["input"] = Input::get()->save_to_json();

        std::ofstream out(proj_file);
        if (out.is_open()) {
            out << root_json.dump(4);
            out.close();
            EditorState::get()->set_status_message("Saved project settings to project.rnode");
            std::cout << "[ProjectSettings] Saved input mappings to " << proj_file << std::endl;
        } else {
            EditorState::get()->set_status_message("Failed to save project settings!");
        }
        is_open = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Close", ImVec2(80.0f, 32.0f))) {
        is_open = false;
    }

    ImGui::End();
}

} // namespace RetroNode
