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

    const auto& open_scenes = EditorState::get()->get_open_scenes();
    std::string current_path = EditorState::get()->get_current_scene_path();

    if (!open_scenes.empty()) {
        if (ImGui::BeginTabBar("SceneTabBar", ImGuiTabBarFlags_Reorderable)) {
            std::string scene_to_open = "";
            std::string scene_to_close = "";

            for (const auto& scene_path : open_scenes) {
                std::string fname = scene_path;
                size_t slash_pos = fname.find_last_of("/\\");
                if (slash_pos != std::string::npos) {
                    fname = fname.substr(slash_pos + 1);
                }

                bool is_selected = (scene_path == current_path);
                ImGuiTabItemFlags tab_flags = is_selected ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;

                bool open = true;
                if (ImGui::BeginTabItem(fname.c_str(), &open, tab_flags)) {
                    if (scene_path != current_path && ImGui::IsItemActivated()) {
                        scene_to_open = scene_path;
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
    }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    int v_w = VisualServer::get()->get_virtual_width();
    int v_h = VisualServer::get()->get_virtual_height();

    if (avail.x > 0 && avail.y > 0 && v_w > 0 && v_h > 0) {
        float target_aspect = (float)v_w / (float)v_h;
        float rendered_w = avail.x;
        float rendered_h = avail.x / target_aspect;

        if (rendered_h > avail.y) {
            rendered_h = avail.y;
            rendered_w = avail.y * target_aspect;
        }

        float pad_x = (avail.x - rendered_w) * 0.5f;
        float pad_y = (avail.y - rendered_h) * 0.5f;

        ImGui::SetCursorPos(ImVec2(pad_x, pad_y));
        ImVec2 v_origin = ImGui::GetCursorScreenPos();

        SDL_Texture* tex = VisualServer::get()->get_framebuffer_texture();
        if (tex) {
            ImGui::Image((ImTextureID)(intptr_t)tex, ImVec2(rendered_w, rendered_h));
        } else {
            ImGui::Dummy(ImVec2(rendered_w, rendered_h));
        }

        float view_scale_x = rendered_w / (float)v_w;
        float view_scale_y = rendered_h / (float)v_h;

        ImGuiIO& io = ImGui::GetIO();
        EditorCamera2D& camera = EditorState::get()->get_camera();
        bool is_hovered = ImGui::IsItemHovered();

        // 1. Camera Panning & Zooming Controls
        if (is_hovered) {
            if (io.MouseWheel != 0.0f) {
                camera.adjust_zoom(io.MouseWheel * 0.15f);
            }
            if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
                camera.pan.x -= Fixed16::from_float(io.MouseDelta.x / (view_scale_x * camera.zoom));
                camera.pan.y -= Fixed16::from_float(io.MouseDelta.y / (view_scale_y * camera.zoom));
            }
        }

        // 2. Gizmos & Selection Outlines
        Node* selected_node = EditorState::get()->get_selected_node();
        Node2D* n2d = dynamic_cast<Node2D*>(selected_node);

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

            // Axis Lines
            draw_list->AddLine(screen_pos, handle_x_end, IM_COL32(235, 60, 60, 255), 2.5f);
            draw_list->AddLine(screen_pos, handle_y_end, IM_COL32(60, 235, 60, 255), 2.5f);

            // Center Box
            draw_list->AddRectFilled(
                ImVec2(screen_pos.x - 4.0f, screen_pos.y - 4.0f),
                ImVec2(screen_pos.x + 4.0f, screen_pos.y + 4.0f),
                IM_COL32(255, 255, 255, 255)
            );

            // Rotate Ring
            draw_list->AddCircle(screen_pos, handle_len + 8.0f, IM_COL32(60, 140, 255, 200), 24, 1.5f);

            // Scale Corner Handle
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

                // Grid Snapping (Ctrl key down -> 16px grid round)
                if (io.KeyCtrl) {
                    float px = n2d->position.x.to_float();
                    float py = n2d->position.y.to_float();
                    n2d->position.x = Fixed16::from_float(std::round(px / 16.0f) * 16.0f);
                    n2d->position.y = Fixed16::from_float(std::round(py / 16.0f) * 16.0f);
                }
            }

            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                active_gizmo_axis = GizmoAxis::NONE;
            }
        }
    }

    ImGui::End();
}

} // namespace RetroNode
