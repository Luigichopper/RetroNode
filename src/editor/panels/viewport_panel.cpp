#include "viewport_panel.h"
#include "../editor_state.h"
#include "../../servers/visual_server.h"
#include "../../scene/2d/node_2d.h"
#include "../../scene/2d/sprite_2d.h"
#include "../../scene/gui/control.h"
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

    bool select_viewport_tab = EditorState::get()->pop_focus_viewport_tab_request();
    const auto& open_scenes = EditorState::get()->get_open_scenes();
    std::string current_path = EditorState::get()->get_current_scene_path();

    if (ImGui::BeginTabBar("ViewportDockTabBar", ImGuiTabBarFlags_Reorderable)) {
        std::string scene_to_open = "";
        std::string scene_to_close = "";

        // ----------------------------------------------------
        // Render Top-Level Tabs for Each Open Scene File
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

                // Render Freer 2D Viewport Editing Controls & Framebuffer
                ImVec2 avail = ImGui::GetContentRegionAvail();

                if (avail.x > 0 && avail.y > 0) {
                    EditorCamera2D& camera = EditorState::get()->get_camera_for_scene(scene_path);
                    Node* cur_selected = EditorState::get()->get_selected_node();
                    Node2D* cur_n2d = dynamic_cast<Node2D*>(cur_selected);
                    Control* cur_ctrl = dynamic_cast<Control*>(cur_selected);

                    // Top Toolbar Header Bar for Camera & Focus Controls (reserved 32px height)
                    float toolbar_h = 32.0f;
                    float scene_view_h = std::max(1.0f, avail.y - toolbar_h);

                    ImGui::SetCursorPos(ImVec2(10.0f, 4.0f));
                    ImGui::BeginGroup();
                    ImGui::Text("Camera: (%.1f, %.1f) | Zoom: %.0f%%", camera.pan.x.to_float(), camera.pan.y.to_float(), camera.zoom * 100.0f);
                    ImGui::SameLine();
                    if (ImGui::Button(" - ")) camera.adjust_zoom(-0.25f);
                    ImGui::SameLine();
                    if (ImGui::Button(" + ")) camera.adjust_zoom(0.25f);
                    ImGui::SameLine();

                    if (ImGui::Button("Focus (F)")) {
                        if (cur_n2d) {
                            camera.pan = cur_n2d->get_global_position();
                        } else if (cur_ctrl) {
                            camera.pan = cur_ctrl->get_global_control_position();
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Reset")) {
                        camera.pan = Vector2Fixed::zero();
                        camera.zoom = 1.0f;
                    }
                    ImGui::EndGroup();

                    // Render scene into editor framebuffer matching available viewport dimensions
                    Node* edit_root = SceneTree::get()->get_root();
                    if (edit_root) {
                        VisualServer::get()->clear_render_queue();
                        edit_root->propagate_process(0.016f);
                    }

                    VisualServer::get()->render_editor_scene(0.0f, (int)avail.x, (int)scene_view_h, camera.pan, camera.zoom);

                    ImGui::SetCursorPos(ImVec2(0.0f, toolbar_h));
                    ImVec2 v_origin = ImGui::GetCursorScreenPos();

                    SDL_Texture* tex = VisualServer::get()->get_editor_framebuffer_texture((int)avail.x, (int)scene_view_h);
                    if (tex) {
                        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
                        ImGui::Image((ImTextureID)(intptr_t)tex, ImVec2(avail.x, scene_view_h));
                    } else {
                        ImGui::Dummy(ImVec2(avail.x, scene_view_h));
                    }

                    ImGuiIO& io = ImGui::GetIO();
                    bool is_hovered = ImGui::IsItemHovered();

                    // Camera Input Controls
                    if (is_hovered) {
                        if (io.MouseWheel != 0.0f) {
                            camera.adjust_zoom(io.MouseWheel * 0.15f);
                        }
                        if (ImGui::IsMouseDown(ImGuiMouseButton_Middle) || ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
                            camera.pan.x -= Fixed16::from_float(io.MouseDelta.x / camera.zoom);
                            camera.pan.y -= Fixed16::from_float(io.MouseDelta.y / camera.zoom);
                        }

                        float pan_speed = 6.0f / camera.zoom;
                        if (ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_LeftArrow)) camera.pan.x -= Fixed16::from_float(pan_speed);
                        if (ImGui::IsKeyDown(ImGuiKey_D) || ImGui::IsKeyDown(ImGuiKey_RightArrow)) camera.pan.x += Fixed16::from_float(pan_speed);
                        if (ImGui::IsKeyDown(ImGuiKey_W) || ImGui::IsKeyDown(ImGuiKey_UpArrow)) camera.pan.y -= Fixed16::from_float(pan_speed);
                        if (ImGui::IsKeyDown(ImGuiKey_S) || ImGui::IsKeyDown(ImGuiKey_DownArrow)) camera.pan.y += Fixed16::from_float(pan_speed);
                        if (ImGui::IsKeyPressed(ImGuiKey_F)) {
                            if (cur_n2d) camera.pan = cur_n2d->get_global_position();
                            else if (cur_ctrl) camera.pan = cur_ctrl->get_global_control_position();
                        }
                    }

                    // Gizmos & Selection Outlines (Supports Node2D and Control nodes like Label, NinePatchRect)
                    if (cur_n2d || cur_ctrl) {
                        Vector2Fixed global_pos = cur_n2d ? cur_n2d->get_global_position() : cur_ctrl->get_global_control_position();
                        ImVec2 center_pos = ImVec2(v_origin.x + avail.x * 0.5f, v_origin.y + scene_view_h * 0.5f);

                        float screen_x = center_pos.x + (global_pos.x.to_float() - camera.pan.x.to_float()) * camera.zoom;
                        float screen_y = center_pos.y + (global_pos.y.to_float() - camera.pan.y.to_float()) * camera.zoom;
                        ImVec2 screen_pos = ImVec2(screen_x, screen_y);

                        Vector2Fixed node_size = Vector2Fixed::from_floats(16.0f, 16.0f);
                        Vector2Fixed global_scale = Vector2Fixed::one();

                        if (cur_n2d) {
                            global_scale = cur_n2d->get_global_scale();
                            Sprite2D* sprite = dynamic_cast<Sprite2D*>(cur_n2d);
                            if (sprite) {
                                node_size = sprite->get_texture_size();
                            }
                        } else if (cur_ctrl) {
                            node_size = cur_ctrl->get_size();
                        }

                        float scaled_w = node_size.x.to_float() * global_scale.x.to_float() * camera.zoom;
                        float scaled_h = node_size.y.to_float() * global_scale.y.to_float() * camera.zoom;

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

                            GizmoAxis chosen_axis = GizmoAxis::NONE;
                            if (dist_center < 6.0f) {
                                chosen_axis = GizmoAxis::CENTER;
                            } else if (dist_scale < 8.0f) {
                                chosen_axis = GizmoAxis::SCALE_X;
                            } else if (std::abs(dist_center - (handle_len + 8.0f)) < 6.0f) {
                                chosen_axis = GizmoAxis::ROTATE;
                            } else if (mouse_pos.x >= screen_pos.x && mouse_pos.x <= handle_x_end.x && dist_x_axis < 6.0f) {
                                chosen_axis = GizmoAxis::X_AXIS;
                            } else if (mouse_pos.y >= screen_pos.y && mouse_pos.y <= handle_y_end.y && dist_y_axis < 6.0f) {
                                chosen_axis = GizmoAxis::Y_AXIS;
                            }

                            if (chosen_axis != GizmoAxis::NONE) {
                                active_gizmo_axis = chosen_axis;
                                drag_start_mouse_pos = mouse_pos;
                                EditorState::get()->push_undo_snapshot();
                            }
                        }

                        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && active_gizmo_axis != GizmoAxis::NONE) {
                            ImVec2 mouse_delta = io.MouseDelta;
                            float world_delta_x = mouse_delta.x / camera.zoom;
                            float world_delta_y = mouse_delta.y / camera.zoom;

                            if (cur_n2d) {
                                if (active_gizmo_axis == GizmoAxis::X_AXIS || active_gizmo_axis == GizmoAxis::CENTER) {
                                    cur_n2d->position.x += Fixed16::from_float(world_delta_x);
                                }
                                if (active_gizmo_axis == GizmoAxis::Y_AXIS || active_gizmo_axis == GizmoAxis::CENTER) {
                                    cur_n2d->position.y += Fixed16::from_float(world_delta_y);
                                }

                                if (active_gizmo_axis == GizmoAxis::ROTATE) {
                                    cur_n2d->rotation += Fixed16::from_float(world_delta_x * 2.0f);
                                }

                                if (active_gizmo_axis == GizmoAxis::SCALE_X) {
                                    cur_n2d->scale.x += Fixed16::from_float(world_delta_x * 0.05f);
                                    cur_n2d->scale.y += Fixed16::from_float(world_delta_y * 0.05f);
                                }

                                if (io.KeyCtrl) {
                                    float px = cur_n2d->position.x.to_float();
                                    float py = cur_n2d->position.y.to_float();
                                    cur_n2d->position.x = Fixed16::from_float(std::round(px / 16.0f) * 16.0f);
                                    cur_n2d->position.y = Fixed16::from_float(std::round(py / 16.0f) * 16.0f);
                                }
                                cur_n2d->previous_position = cur_n2d->position;
                            } else if (cur_ctrl) {
                                if (active_gizmo_axis == GizmoAxis::X_AXIS || active_gizmo_axis == GizmoAxis::CENTER) {
                                    cur_ctrl->position.x += Fixed16::from_float(world_delta_x);
                                }
                                if (active_gizmo_axis == GizmoAxis::Y_AXIS || active_gizmo_axis == GizmoAxis::CENTER) {
                                    cur_ctrl->position.y += Fixed16::from_float(world_delta_y);
                                }

                                if (active_gizmo_axis == GizmoAxis::SCALE_X) {
                                    cur_ctrl->size.x += Fixed16::from_float(world_delta_x);
                                    cur_ctrl->size.y += Fixed16::from_float(world_delta_y);
                                }

                                if (io.KeyCtrl) {
                                    float px = cur_ctrl->position.x.to_float();
                                    float py = cur_ctrl->position.y.to_float();
                                    cur_ctrl->position.x = Fixed16::from_float(std::round(px / 16.0f) * 16.0f);
                                    cur_ctrl->position.y = Fixed16::from_float(std::round(py / 16.0f) * 16.0f);
                                }
                                cur_ctrl->previous_position = cur_ctrl->position;
                            }
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
