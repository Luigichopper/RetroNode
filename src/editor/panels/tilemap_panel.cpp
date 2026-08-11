#include "tilemap_panel.h"
#include "../editor_state.h"
#include "../../scene/2d/tile_map_layer.h"
#include "../../servers/texture_server.h"
#include <imgui.h>
#include <iostream>
#include <queue>
#include <algorithm>

namespace RetroNode {

TileTool TilemapPanel::current_tool = TileTool::PENCIL;
int TilemapPanel::active_tile_index = 0;
bool TilemapPanel::paint_solid = false;
unsigned int TilemapPanel::active_metadata = 0;
bool TilemapPanel::show_grid_overlay = true;
bool TilemapPanel::show_collision_overlay = true;

bool TilemapPanel::is_rect_drag = false;
int TilemapPanel::rect_start_col = 0;
int TilemapPanel::rect_start_row = 0;

void TilemapPanel::perform_flood_fill(TileMapLayer* tilemap, int start_col, int start_row, int fill_tile, bool solid, uint32_t metadata) {
    if (!tilemap) return;
    int cols = tilemap->columns;
    int rows = tilemap->rows;
    if (start_col < 0 || start_col >= cols || start_row < 0 || start_row >= rows) return;

    int target_tile = tilemap->get_cell(start_col, start_row);
    if (target_tile == fill_tile && current_tool != TileTool::ERASER) return;

    std::queue<std::pair<int, int>> q;
    std::vector<bool> visited(cols * rows, false);

    q.push({start_col, start_row});
    visited[start_row * cols + start_col] = true;

    while (!q.empty()) {
        auto [c, r] = q.front();
        q.pop();

        int current_tile = tilemap->get_cell(c, r);
        if (current_tile != target_tile) continue;

        if (current_tool == TileTool::ERASER) {
            tilemap->set_cell(c, r, -1, false, 0);
        } else {
            tilemap->set_cell(c, r, fill_tile, solid, metadata);
        }

        const int dc[4] = { 1, -1, 0, 0 };
        const int dr[4] = { 0, 0, 1, -1 };

        for (int i = 0; i < 4; ++i) {
            int nc = c + dc[i];
            int nr = r + dr[i];
            if (nc >= 0 && nc < cols && nr >= 0 && nr < rows) {
                int idx = nr * cols + nc;
                if (!visited[idx] && tilemap->get_cell(nc, nr) == target_tile) {
                    visited[idx] = true;
                    q.push({nc, nr});
                }
            }
        }
    }
}

void TilemapPanel::perform_rect_fill(TileMapLayer* tilemap, int col1, int row1, int col2, int row2, int fill_tile, bool solid, uint32_t metadata) {
    if (!tilemap) return;
    int min_c = std::clamp(std::min(col1, col2), 0, tilemap->columns - 1);
    int max_c = std::clamp(std::max(col1, col2), 0, tilemap->columns - 1);
    int min_r = std::clamp(std::min(row1, row2), 0, tilemap->rows - 1);
    int max_r = std::clamp(std::max(row1, row2), 0, tilemap->rows - 1);

    for (int r = min_r; r <= max_r; ++r) {
        for (int c = min_c; c <= max_c; ++c) {
            if (current_tool == TileTool::ERASER) {
                tilemap->set_cell(c, r, -1, false, 0);
            } else {
                tilemap->set_cell(c, r, fill_tile, solid, metadata);
            }
        }
    }
}

void TilemapPanel::draw() {
    ImGui::Begin("🎨 Tilemap Painter");

    Node* sel = EditorState::get()->get_selected_node();
    TileMapLayer* tilemap = dynamic_cast<TileMapLayer*>(sel);

    if (!tilemap) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Select a TileMapLayer node in Scene Tree");
        ImGui::TextDisabled("to edit tiles and paint visually in the viewport.");
        ImGui::End();
        return;
    }

    ImGui::TextColored(ImVec4(0.3f, 0.75f, 1.0f, 1.0f), "Editing: %s", tilemap->get_name().c_str());
    ImGui::TextDisabled("Grid: %d x %d | Tile Size: %dpx", tilemap->columns, tilemap->rows, tilemap->tile_size);
    ImGui::Separator();
    ImGui::Spacing();

    // Tool Selector Bar
    ImGui::Text("Paint Tool:");
    if (ImGui::RadioButton("🖌️ Pencil", current_tool == TileTool::PENCIL)) {
        current_tool = TileTool::PENCIL;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("🧹 Eraser", current_tool == TileTool::ERASER)) {
        current_tool = TileTool::ERASER;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("🪣 Bucket", current_tool == TileTool::BUCKET_FILL)) {
        current_tool = TileTool::BUCKET_FILL;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("🔲 Box", current_tool == TileTool::RECT_FILL)) {
        current_tool = TileTool::RECT_FILL;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("🧱 Solid Only", current_tool == TileTool::COLLISION_TOGGLE)) {
        current_tool = TileTool::COLLISION_TOGGLE;
    }

    ImGui::Spacing();
    ImGui::Checkbox("🧱 Paint Solid (Collision)", &paint_solid);
    ImGui::SameLine();
    ImGui::Checkbox("Grid Overlay", &show_grid_overlay);
    ImGui::SameLine();
    ImGui::Checkbox("Collision Overlay", &show_collision_overlay);

    int meta_val = static_cast<int>(active_metadata);
    if (ImGui::InputInt("Cell Metadata", &meta_val)) {
        if (meta_val < 0) meta_val = 0;
        active_metadata = static_cast<unsigned int>(meta_val);
    }

    ImGui::Separator();

    // Map Dimensions & Quick Actions
    if (ImGui::TreeNode("Map Dimensions & Utilities")) {
        static int edit_cols = tilemap->columns;
        static int edit_rows = tilemap->rows;
        static int edit_size = tilemap->tile_size;

        ImGui::InputInt("Columns", &edit_cols);
        ImGui::InputInt("Rows", &edit_rows);
        ImGui::InputInt("Tile Size", &edit_size);

        if (ImGui::Button("Resize Map", ImVec2(120.0f, 24.0f))) {
            if (edit_cols > 0 && edit_rows > 0 && edit_size > 0) {
                EditorState::get()->push_undo_snapshot();
                tilemap->columns = edit_cols;
                tilemap->rows = edit_rows;
                tilemap->tile_size = edit_size;
                tilemap->setup_map(edit_cols, edit_rows, edit_size, tilemap->tile_data, tilemap->collision_data);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear All", ImVec2(90.0f, 24.0f))) {
            EditorState::get()->push_undo_snapshot();
            std::fill(tilemap->tile_data.begin(), tilemap->tile_data.end(), -1);
            std::fill(tilemap->collision_data.begin(), tilemap->collision_data.end(), false);
        }
        ImGui::SameLine();
        if (ImGui::Button("Fill All", ImVec2(80.0f, 24.0f))) {
            EditorState::get()->push_undo_snapshot();
            std::fill(tilemap->tile_data.begin(), tilemap->tile_data.end(), active_tile_index);
            std::fill(tilemap->collision_data.begin(), tilemap->collision_data.end(), paint_solid);
        }
        ImGui::TreePop();
    }

    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Tileset Palette:");

    uint32_t tex_id = tilemap->tileset_texture_id;
    if (tex_id == 0 && !tilemap->tileset_path.empty()) {
        tex_id = TextureServer::get()->load_texture(tilemap->tileset_path);
    }

    SDL_Texture* tex = TextureServer::get()->get_texture(tex_id);
    if (!tex) {
        ImGui::TextDisabled("No valid tileset texture loaded.");
        ImGui::Text("Set 'tileset' property in Inspector.");
        ImGui::End();
        return;
    }

    Vector2Fixed tex_sz = TextureServer::get()->get_texture_size(tex_id);
    int tw = tex_sz.x.to_int();
    int th = tex_sz.y.to_int();
    int tile_sz = tilemap->tile_size;
    int cols = (tw > 0 && tile_sz > 0) ? (tw / tile_sz) : 1;
    int rows = (th > 0 && tile_sz > 0) ? (th / tile_sz) : 1;
    int total_tiles = cols * rows;

    ImGui::TextDisabled("Tileset: %s (%dx%d, %d tiles)", tilemap->tileset_path.c_str(), tw, th, total_tiles);
    ImGui::Spacing();

    // Tileset Palette Grid Preview
    float zoom_scale = 2.0f;
    float preview_tile_sz = static_cast<float>(tile_sz) * zoom_scale;

    ImVec2 content_avail = ImGui::GetContentRegionAvail();
    int display_cols = std::max(1, (int)(content_avail.x / (preview_tile_sz + 4.0f)));

    for (int i = 0; i < total_tiles; ++i) {
        int tile_x = (i % cols) * tile_sz;
        int tile_y = (i / cols) * tile_sz;

        ImVec2 uv0 = ImVec2((float)tile_x / tw, (float)tile_y / th);
        ImVec2 uv1 = ImVec2((float)(tile_x + tile_sz) / tw, (float)(tile_y + tile_sz) / th);

        std::string btn_id = "##tile_btn_" + std::to_string(i);

        bool is_selected = (current_tool != TileTool::ERASER && active_tile_index == i);
        if (is_selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.9f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.85f, 0.3f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
        }

        if (ImGui::ImageButton(btn_id.c_str(), (ImTextureID)(intptr_t)tex, ImVec2(preview_tile_sz, preview_tile_sz), uv0, uv1)) {
            active_tile_index = i;
            if (current_tool == TileTool::ERASER) {
                current_tool = TileTool::PENCIL;
            }
        }

        if (is_selected) {
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(2);
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Tile Index: %d (Col: %d, Row: %d)", i, i % cols, i / cols);
        }

        if ((i + 1) % display_cols != 0 && i < total_tiles - 1) {
            ImGui::SameLine();
        }
    }

    ImGui::End();
}

} // namespace RetroNode
