#include "viewport_panel.h"
#include "../editor_state.h"
#include "../../servers/visual_server.h"
#include "../../scene/2d/node_2d.h"
#include "../../scene/2d/sprite_2d.h"
#include <imgui.h>
#include <cmath>
#include <iostream>

namespace RetroNode {

enum class GizmoAxis { NONE, X_AXIS, Y_AXIS, CENTER, ROTATE, SCALE_X, SCALE_Y };
static GizmoAxis active_gizmo_axis = GizmoAxis::NONE;
static Vector2Fixed drag_start_node_pos;
static ImVec2 drag_start_mouse_pos;

void ViewportPanel::draw() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport", NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();

    bool select_game_tab = EditorState::get()->pop_focus_game_tab_request();
    bool select_viewport_tab = EditorState::get()->pop_focus_viewport_tab_request();

    const auto& open_scenes = EditorState::get()->get_open_scenes();
    std::string current_path = EditorState::get()->get_current_scene_path();

    if (ImGui::BeginTabBar("ViewportDockTabBar", ImGuiTabBarFlags_Reorderable)) {
        std::string scene_to_open = "";
        std::string scene_to_close = "";

        // ----------------------------------------------------
        // 1. Render Top-Level Tabs for Each Open Scene File
        // ----------------------------------------------------
        for (const auto& scene_path : open_scenes) {
            std::string fname = scene_path;
            size_t slash_pos = fname.find_last_of("/\\");
            if (slash_pos != std::string::npos) {
                fname = fname.substr(slash_pos + 1);
            }

            std::string tab_label = "📄 " + fname;
            bool is_active_scene = (scene_path == current_path);
            ImGuiTabItemFlags tab_flags = (is_active_scene && select_viewport_tab) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;

            bool open = true;
            if (ImGui::BeginTabItem(tab_label.c_str(), &open, tab_flags)) {
                if (!is_active_scene && ImGui::IsItemActivated()) {
                    scene_to_open = scene_path;
                }

                // Render 2D Viewport Editing Controls & Framebuffer
                ImVec2 avail = ImGui::GetContentRegionAvail();
                int v_w = VisualServer::get()->get_virtual_width();
                int v_h = VisualServer::get()->get_virtual_height();

                if (avail.x > 0 && avail.y > 0 && v_w > 0 && v_h > 0) {
                    int scale_factor = std::max(1, (int)std::min(avail.x / (float)v_w, avail.y / (float)v_h));
                    float rendered_w = static_cast<float>(v_w * scale_factor);
                    float rendered_h = static_cast<float>(v_h * scale_factor);

                    float pad_x = std::floor((avail.x - rendered_w) * 0.5f);
                    float pad_y = std::floor((avail.y - rendered_h) * 0.5f);

                    ImGui::SetCursorPos(ImVec2(pad_x, pad_y));
                    ImVec2 v_origin = ImGui::GetCursorScreenPos();

                    SDL_Texture* tex = VisualServer::get()->get_framebuffer_texture();
                    if (tex) {
                        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
                        ImGui::Image((ImTextureID)(intptr_t)tex, ImVec2(rendered_w, rendered_h));
                    } else {
                        ImGui::Dummy(ImVec2(rendered_w, rendered_h));
                    }

                    float view_scale_x = static_cast<float>(scale_factor);
                    float view_scale_y = static_cast<float>(scale_factor);

                    ImGuiIO& io = ImGui::GetIO();
                    EditorCamera2D& camera = EditorState::get()->get_camera();
                    bool is_hovered = ImGui::IsItemHovered();

                    // Camera Controls
                    if (is_hovered) {
                        if (io.MouseWheel != 0.0f) {
                            camera.adjust_zoom(io.MouseWheel * 0.15f);
                        }
                        if (ImGui::IsMouseDown(ImGuiMouseButton_Middle) || ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
                            camera.pan.x -= Fixed16::from_float(io.MouseDelta.x / (view_scale_x * camera.zoom));
                            camera.pan.y -= Fixed16::from_float(io.MouseDelta.y / (view_scale_y * camera.zoom));
                        }

                        float pan_speed = 6.0f / (view_scale_x * camera.zoom);
                        if (ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_LeftArrow)) camera.pan.x -= Fixed16::from_float(pan_speed);
                        if (ImGui::IsKeyDown(ImGuiKey_D) || ImGui::IsKeyDown(ImGuiKey_RightArrow)) camera.pan.x += Fixed16::from_float(pan_speed);
                        if (ImGui::IsKeyDown(ImGuiKey_W) || ImGui::IsKeyDown(ImGuiKey_UpArrow)) camera.pan.y -= Fixed16::from_float(pan_speed);
                        if (ImGui::IsKeyDown(ImGuiKey_S) || ImGui::IsKeyDown(ImGuiKey_DownArrow)) camera.pan.y += Fixed16::from_float(pan_speed);
                    }

                    // Floating Camera Overlay Toolbar
                    ImGui::SetCursorPos(ImVec2(pad_x + 10.0f, pad_y + 10.0f));
                    ImGui::BeginGroup();
                    ImGui::Text("Camera: (%.1f, %.1f) | Zoom: %.0f%%", camera.pan.x.to_float(), camera.pan.y.to_float(), camera.zoom * 100.0f);
                    ImGui::SameLine();
                    if (ImGui::Button(" - ")) camera.adjust_zoom(-0.25f);
                    ImGui::SameLine();
                    if (ImGui::Button(" + ")) camera.adjust_zoom(0.25f);
                    ImGui::SameLine();

                    Node* cur_selected = EditorState::get()->get_selected_node();
                    Node2D* cur_n2d = dynamic_cast<Node2D*>(cur_selected);

                    if (ImGui::Button("Focus (F)") || (is_hovered && ImGui::IsKeyPressed(ImGuiKey_F))) {
                        if (cur_n2d) {
                            camera.pan = cur_n2d->get_global_position();
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Reset")) {
                        camera.pan = Vector2Fixed::zero();
                        camera.zoom = 1.0f;
                    }
                    ImGui::EndGroup();

                    // Gizmos & Selection Outlines
                    Node2D* n2d = cur_n2d;

                    if (n2d) {
                        Vector2Fixed global_pos = n2d->get_global_position();
                        float screen_x = v_origin.x + (global_pos.x.to_float() - camera.pan.x.to_float()) * view_scale_x * camera.zoom;
                        float screen_y = v_origin.y + (global_pos.y.to_float() - camera.pan.y.to_float()) * view_scale_y * camera.zoom;
                        ImVec2 screen_pos = ImVec2(screen_x, screen_y);

                        Vector2Fixed node_size = Vector2Fixed::from_floats(16.0f, 16.0f);
                        Sprite2D* sprite = dynamic_cast<Sprite2D*>(n2d);
                        if (sprite) {
                            node_size = sprite->get_texture_size();
                        }

                        Vector2Fixed global_scale = n2d->get_global_scale();
                        float scaled_w = node_size.x.to_float() * global_scale.x.to_float() * view_scale_x * camera.zoom;
                        float scaled_h = node_size.y.to_float() * global_scale.y.to_float() * view_scale_y * camera.zoom;

                        ImVec2 bbox_min = screen_pos;
                        ImVec2 bbox_max = ImVec2(screen_pos.x + scaled_w, screen_pos.y + scaled_h);

                        ImDrawList* draw_list = ImGui::GetWindowDrawList();

                        // Yellow Bounding Box Outline
                        draw_list->AddRect(bbox_min, bbox_max, IM_COL32(255, 220, 0, 255), 0.0f, 0, 1.5f);

                        // Move Gizmo Handles (Red = X, Green = Y)
                        float handle_len = 36.0f;
                        ImVec2 handle_x_end = ImVec2(screen_pos.x + handle_len, screen_pos.y);
                        ImVec2 handle_y_end = ImVec2(screen_pos.x, screen_pos.y + handle_len);

                        draw_list->AddLine(screen_pos, handle_x_end, IM_COL32(235, 60, 60, 255), 2.5f);
                        draw_list->AddLine(screen_pos, handle_y_end, IM_COL32(60, 235, 60, 255), 2.5f);

                        draw_list->AddRectFilled(
                            ImVec2(screen_pos.x - 4.0f, screen_pos.y - 4.0f),
                            ImVec2(screen_pos.x + 4.0f, screen_pos.y + 4.0f),
                            IM_COL32(255, 255, 255, 255)
                        );

                        draw_list->AddCircle(screen_pos, handle_len + 8.0f, IM_COL32(60, 140, 255, 200), 24, 1.5f);

                        draw_list->AddRectFilled(
                            ImVec2(bbox_max.x - 4.0f, bbox_max.y - 4.0f),
                            ImVec2(bbox_max.x + 4.0f, bbox_max.y + 4.0f),
                            IM_COL32(60, 140, 255, 255)
                        );

                        // Gizmo Drag Interaction
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && is_hovered) {
                            ImVec2 mouse_pos = io.MousePos;

                            float dist_center = std::hypot(mouse_pos.x - screen_pos.x, mouse_pos.y - screen_pos.y);
                            float dist_x_axis = std::abs(mouse_pos.y - screen_pos.y);
                            float dist_y_axis = std::abs(mouse_pos.x - screen_pos.x);
                            float dist_scale = std::hypot(mouse_pos.x - bbox_max.x, mouse_pos.y - bbox_max.y);

                            if (dist_center < 6.0f) {
                                active_gizmo_axis = GizmoAxis::CENTER;
                                drag_start_node_pos = n2d->position;
                                drag_start_mouse_pos = mouse_pos;
                            } else if (dist_scale < 8.0f) {
                                active_gizmo_axis = GizmoAxis::SCALE_X;
                                drag_start_node_pos = n2d->position;
                                drag_start_mouse_pos = mouse_pos;
                            } else if (std::abs(dist_center - (handle_len + 8.0f)) < 6.0f) {
                                active_gizmo_axis = GizmoAxis::ROTATE;
                                drag_start_node_pos = n2d->position;
                                drag_start_mouse_pos = mouse_pos;
                            } else if (mouse_pos.x >= screen_pos.x && mouse_pos.x <= handle_x_end.x && dist_x_axis < 6.0f) {
                                active_gizmo_axis = GizmoAxis::X_AXIS;
                                drag_start_node_pos = n2d->position;
                                drag_start_mouse_pos = mouse_pos;
                            } else if (mouse_pos.y >= screen_pos.y && mouse_pos.y <= handle_y_end.y && dist_y_axis < 6.0f) {
                                active_gizmo_axis = GizmoAxis::Y_AXIS;
                                drag_start_node_pos = n2d->position;
                                drag_start_mouse_pos = mouse_pos;
                            }
                        }

                        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && active_gizmo_axis != GizmoAxis::NONE) {
                            ImVec2 mouse_delta = io.MouseDelta;
                            float world_delta_x = mouse_delta.x / (view_scale_x * camera.zoom);
                            float world_delta_y = mouse_delta.y / (view_scale_y * camera.zoom);

                            if (active_gizmo_axis == GizmoAxis::X_AXIS || active_gizmo_axis == GizmoAxis::CENTER) {
                                n2d->position.x += Fixed16::from_float(world_delta_x);
                            }
                            if (active_gizmo_axis == GizmoAxis::Y_AXIS || active_gizmo_axis == GizmoAxis::CENTER) {
                                n2d->position.y += Fixed16::from_float(world_delta_y);
                            }

                            if (active_gizmo_axis == GizmoAxis::ROTATE) {
                                n2d->rotation += Fixed16::from_float(world_delta_x * 2.0f);
                            }

                            if (active_gizmo_axis == GizmoAxis::SCALE_X) {
                                n2d->scale.x += Fixed16::from_float(world_delta_x * 0.05f);
                                n2d->scale.y += Fixed16::from_float(world_delta_y * 0.05f);
                            }

                            if (io.KeyCtrl) {
                                float px = n2d->position.x.to_float();
                                float py = n2d->position.y.to_float();
                                n2d->position.x = Fixed16::from_float(std::round(px / 16.0f) * 16.0f);
                                n2d->position.y = Fixed16::from_float(std::round(py / 16.0f) * 16.0f);
                            }
                            n2d->previous_position = n2d->position;
                            EditorState::get()->push_undo_snapshot();
                        }

                        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                            active_gizmo_axis = GizmoAxis::NONE;
                        }
                    }
                }
                ImGui::EndTabItem();
            }

            if (!open) {
                scene_to_close = scene_path;
            }
        }

        // ----------------------------------------------------
        // 2. Render Dedicated Game View Tab (Main Scene Execution)
        // ----------------------------------------------------
        bool is_playing = EditorState::get()->get_is_play_mode();
        std::string game_tab_title = is_playing ? "🎮 Game View (Running)" : "🎮 Game View";
        ImGuiTabItemFlags game_flags = select_game_tab ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;

        if (ImGui::BeginTabItem(game_tab_title.c_str(), NULL, game_flags)) {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            int v_w = VisualServer::get()->get_virtual_width();
            int v_h = VisualServer::get()->get_virtual_height();

            if (avail.x > 0 && avail.y > 0 && v_w > 0 && v_h > 0) {
                int scale_factor = std::max(1, (int)std::min(avail.x / (float)v_w, (avail.y - 32.0f) / (float)v_h));
                float rendered_w = static_cast<float>(v_w * scale_factor);
                float rendered_h = static_cast<float>(v_h * scale_factor);

                float pad_x = std::floor((avail.x - rendered_w) * 0.5f);
                float pad_y = std::floor(((avail.y - 32.0f) - rendered_h) * 0.5f + 32.0f);

                // Professional Header Banner for Gameplay Window
                ImGui::SetCursorPos(ImVec2(10.0f, 6.0f));
                ImGui::BeginGroup();
                if (is_playing) {
                    ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "🎮 ACTIVE GAMEPLAY SESSION");
                    ImGui::SameLine();
                    ImGui::Text(" | Main Scene: %s | FPS: 60", EditorState::get()->get_main_scene_path().c_str());
                    ImGui::SameLine(avail.x - 140.0f);
                    if (ImGui::Button("⏹️ Stop Game")) {
                        EditorState::get()->stop_play_mode();
                    }
                } else {
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "🎮 GAMEPLAY INACTIVE");
                    ImGui::SameLine();
                    ImGui::Text(" | Press Play (F5) to launch %s", EditorState::get()->get_main_scene_path().c_str());
                    ImGui::SameLine(avail.x - 140.0f);
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
            EditorState::get()->set_game_view_active(true);
            ImGui::EndTabItem();
        } else {
            EditorState::get()->set_game_view_active(false);
        }

        ImGui::EndTabBar();

        if (!scene_to_close.empty()) {
            EditorState::get()->close_open_scene(scene_to_close);
        }
        if (!scene_to_open.empty()) {
            EditorState::get()->open_scene(scene_to_open);
        }
    }

    ImGui::End();
}

} // namespace RetroNode
