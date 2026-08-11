#include "tilemap_panel.h"
#include "../editor_state.h"
#include "../../scene/2d/tile_map_layer.h"
#include "../../servers/texture_server.h"
#include <imgui.h>
#include <iostream>

namespace RetroNode {

int TilemapPanel::active_tile_index = 0;
bool TilemapPanel::paint_solid = false;
unsigned int TilemapPanel::active_metadata = 0;
bool TilemapPanel::is_eraser = false;

void TilemapPanel::draw() {
    ImGui::Begin("🎨 Tilemap Painter");

    Node* sel = EditorState::get()->get_selected_node();
    TileMapLayer* tilemap = dynamic_cast<TileMapLayer*>(sel);

    if (!tilemap) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Select a TileMapLayer node in Scene Tree");
        ImGui::TextDisabled("to paint tiles visually.");
        ImGui::End();
        return;
    }

    ImGui::Text("Editing TileMap: %s", tilemap->get_name().c_str());
    ImGui::Text("Dimensions: %d x %d (Tile Size: %dpx)", tilemap->columns, tilemap->rows, tilemap->tile_size);
    ImGui::Separator();
    ImGui::Spacing();

    // Tool Selector Bar
    if (ImGui::RadioButton("🖌️ Brush", !is_eraser)) {
        is_eraser = false;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("🧹 Eraser", is_eraser)) {
        is_eraser = true;
    }

    ImGui::Checkbox("🧱 Paint Solid (Collision)", &paint_solid);

    int meta_val = static_cast<int>(active_metadata);
    if (ImGui::InputInt("Cell Metadata", &meta_val)) {
        if (meta_val < 0) meta_val = 0;
        active_metadata = static_cast<unsigned int>(meta_val);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Custom integer metadata / flags assigned to painted tile cells.");
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

    ImGui::Text("Tileset: %s (%dx%d px, %d tiles)", tilemap->tileset_path.c_str(), tw, th, total_tiles);
    ImGui::Spacing();

    // Tileset Palette Grid Preview
    float zoom_scale = 2.0f; // Scale tile preview size for easier clicking
    float preview_tile_sz = static_cast<float>(tile_sz) * zoom_scale;

    ImVec2 content_avail = ImGui::GetContentRegionAvail();
    int display_cols = std::max(1, (int)(content_avail.x / (preview_tile_sz + 4.0f)));

    for (int i = 0; i < total_tiles; ++i) {
        int tile_x = (i % cols) * tile_sz;
        int tile_y = (i / cols) * tile_sz;

        ImVec2 uv0 = ImVec2((float)tile_x / tw, (float)tile_y / th);
        ImVec2 uv1 = ImVec2((float)(tile_x + tile_sz) / tw, (float)(tile_y + tile_sz) / th);

        std::string btn_id = "##tile_btn_" + std::to_string(i);

        bool is_selected = (!is_eraser && active_tile_index == i);
        if (is_selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.9f, 1.0f));
        }

        if (ImGui::ImageButton(btn_id.c_str(), (ImTextureID)(intptr_t)tex, ImVec2(preview_tile_sz, preview_tile_sz), uv0, uv1)) {
            active_tile_index = i;
            is_eraser = false;
        }

        if (is_selected) {
            ImGui::PopStyleColor();
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Tile Index: %d", i);
        }

        if ((i + 1) % display_cols != 0 && i < total_tiles - 1) {
            ImGui::SameLine();
        }
    }

    ImGui::End();
}

} // namespace RetroNode
