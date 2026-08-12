#include "sprite_frames_panel.h"
#include "../editor_state.h"
#include "../file_dialog.h"
#include "../../servers/texture_server.h"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <iostream>
#include <algorithm>
#include <cmath>

namespace RetroNode {

bool SpriteFramesPanel::open = true;
std::string SpriteFramesPanel::active_anim_name = "default";
int SpriteFramesPanel::active_frame_index = 0;
bool SpriteFramesPanel::preview_playing = false;
float SpriteFramesPanel::preview_timer = 0.0f;
int SpriteFramesPanel::preview_frame = 0;

bool SpriteFramesPanel::show_sheet_modal = false;
std::string SpriteFramesPanel::sheet_texture_path = "";
int SpriteFramesPanel::slice_mode = 0;
int SpriteFramesPanel::sheet_cols = 4;
int SpriteFramesPanel::sheet_rows = 1;
int SpriteFramesPanel::frame_width = 16;
int SpriteFramesPanel::frame_height = 16;
int SpriteFramesPanel::separation_x = 0;
int SpriteFramesPanel::separation_y = 0;
int SpriteFramesPanel::offset_x = 0;
int SpriteFramesPanel::offset_y = 0;
std::vector<bool> SpriteFramesPanel::cell_selection;

static AnimationFrame copied_frame_buffer;
static bool has_copied_frame = false;
static float thumbnail_size = 64.0f;
static char anim_filter[128] = "";
static char new_anim_name_buf[128] = "";
static char rename_buf[128] = "";
static bool show_add_anim_popup = false;
static bool show_rename_anim_popup = false;
static char add_file_buf[256] = "";
static bool show_add_file_popup = false;

void SpriteFramesPanel::open_sheet_modal(const std::string& default_path) {
    show_sheet_modal = true;
    if (!default_path.empty()) {
        sheet_texture_path = default_path;
    }
    cell_selection.clear();
}

void SpriteFramesPanel::draw() {
    if (!open) return;

    ImGui::SetNextWindowSize(ImVec2(750, 320), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("SpriteFrames", &open)) {
        ImGui::End();
        return;
    }

    AnimatedSprite2D* anim_sprite = dynamic_cast<AnimatedSprite2D*>(EditorState::get()->get_selected_node());
    if (!anim_sprite) {
        ImGui::TextDisabled("Select an AnimatedSprite2D node in the Scene Tree to edit its SpriteFrames.");
        ImGui::End();
        return;
    }

    SpriteFrames* sf = anim_sprite->get_sprite_frames();
    if (!sf) {
        ImGui::TextDisabled("No SpriteFrames resource bound to target node.");
        ImGui::End();
        return;
    }

    // Ensure active_anim_name is valid
    std::vector<std::string> anim_names = sf->get_animation_names();
    if (anim_names.empty()) {
        sf->add_animation("default");
        anim_names = sf->get_animation_names();
    }

    if (!sf->has_animation(active_anim_name)) {
        active_anim_name = anim_names[0];
        active_frame_index = 0;
    }

    // Main 2-column layout
    ImGui::Columns(2, "SpriteFramesColumns", true);
    static bool init_col_width = false;
    if (!init_col_width) {
        ImGui::SetColumnWidth(0, 240.0f);
        init_col_width = true;
    }

    // Left Column: Animations List
    draw_animations_list(anim_sprite, sf);

    ImGui::NextColumn();

    // Right Column: Animation Frames View & Playback
    draw_frames_view(anim_sprite, sf);

    ImGui::Columns(1);

    // Modal Popup: Add Frames from Sprite Sheet
    if (show_sheet_modal) {
        draw_sprite_sheet_modal(anim_sprite, sf);
    }

    ImGui::End();
}

void SpriteFramesPanel::draw_animations_list(AnimatedSprite2D* node, SpriteFrames* sf) {
    ImGui::TextUnformatted("Animations:");
    ImGui::Separator();

    // Toolbar for Animations
    if (ImGui::Button("+##AddAnim", ImVec2(24, 24))) {
        snprintf(new_anim_name_buf, sizeof(new_anim_name_buf), "new_animation_%d", static_cast<int>(sf->get_animation_names().size()));
        show_add_anim_popup = true;
        ImGui::OpenPopup("Add Animation##Popup");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add New Animation");

    ImGui::SameLine();
    if (ImGui::Button("-##DelAnim", ImVec2(24, 24))) {
        if (sf->get_animation_names().size() > 1) {
            sf->remove_animation(active_anim_name);
            active_anim_name = sf->get_animation_names()[0];
            active_frame_index = 0;
            EditorState::get()->push_undo_snapshot();
        }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Delete Current Animation");

    ImGui::SameLine();
    if (ImGui::Button("R##RenameAnim", ImVec2(24, 24))) {
        snprintf(rename_buf, sizeof(rename_buf), "%s", active_anim_name.c_str());
        show_rename_anim_popup = true;
        ImGui::OpenPopup("Rename Animation##Popup");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rename Animation");

    // Autoplay toggle
    ImGui::SameLine();
    bool is_autoplay = (node->get_autoplay() == active_anim_name);
    if (is_autoplay) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.3f, 1.0f));
    }
    if (ImGui::Button("A##AutoplayToggle", ImVec2(24, 24))) {
        if (is_autoplay) {
            node->set_autoplay("");
        } else {
            node->set_autoplay(active_anim_name);
        }
        EditorState::get()->push_undo_snapshot();
    }
    if (is_autoplay) {
        ImGui::PopStyleColor();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip(is_autoplay ? "Autoplay ON for this animation" : "Set as Autoplay Animation");

    // Loop toggle
    ImGui::SameLine();
    bool loop = sf->get_animation_loop(active_anim_name);
    if (loop) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
    }
    if (ImGui::Button("L##LoopToggle", ImVec2(24, 24))) {
        sf->set_animation_loop(active_anim_name, !loop);
        EditorState::get()->push_undo_snapshot();
    }
    if (loop) {
        ImGui::PopStyleColor();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip(loop ? "Looping ON" : "Looping OFF");

    // FPS input
    ImGui::SameLine();
    float fps = sf->get_animation_speed(active_anim_name);
    ImGui::SetNextItemWidth(60.0f);
    if (ImGui::DragFloat("##FPS", &fps, 0.5f, 0.1f, 120.0f, "%.1f FPS")) {
        sf->set_animation_speed(active_anim_name, fps);
        EditorState::get()->push_undo_snapshot();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Animation Speed (Frames Per Second)");

    // Modal Popup: Add Animation
    if (ImGui::BeginPopupModal("Add Animation##Popup", &show_add_anim_popup, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Name", new_anim_name_buf, sizeof(new_anim_name_buf));
        if (ImGui::Button("Create", ImVec2(80, 0))) {
            if (new_anim_name_buf[0] != '\0') {
                sf->add_animation(new_anim_name_buf);
                active_anim_name = new_anim_name_buf;
                active_frame_index = 0;
                node->set_animation(active_anim_name);
                EditorState::get()->push_undo_snapshot();
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(80, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Modal Popup: Rename Animation
    if (ImGui::BeginPopupModal("Rename Animation##Popup", &show_rename_anim_popup, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("New Name", rename_buf, sizeof(rename_buf));
        if (ImGui::Button("Rename", ImVec2(80, 0))) {
            if (rename_buf[0] != '\0' && std::string(rename_buf) != active_anim_name) {
                std::string old_name = active_anim_name;
                sf->rename_animation(old_name, rename_buf);
                active_anim_name = rename_buf;
                if (node->get_animation() == old_name) {
                    node->set_animation(active_anim_name);
                }
                if (node->get_autoplay() == old_name) {
                    node->set_autoplay(active_anim_name);
                }
                EditorState::get()->push_undo_snapshot();
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(80, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Filter box
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##FilterAnims", "Filter Animations...", anim_filter, sizeof(anim_filter));

    // Scrollable Animations List
    ImGui::BeginChild("AnimationsListChild", ImVec2(0, 0), true);
    std::vector<std::string> anim_names = sf->get_animation_names();
    std::string filter_str = anim_filter;
    std::transform(filter_str.begin(), filter_str.end(), filter_str.begin(), [](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });

    for (const auto& name : anim_names) {
        if (!filter_str.empty()) {
            std::string lower_name = name;
            std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), [](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });
            if (lower_name.find(filter_str) == std::string::npos) continue;
        }

        bool is_selected = (name == active_anim_name);
        int frame_count = sf->get_frame_count(name);
        std::string display = name + " (" + std::to_string(frame_count) + ")";
        if (node->get_autoplay() == name) {
            display = "[A] " + display;
        }

        if (ImGui::Selectable(display.c_str(), is_selected)) {
            active_anim_name = name;
            active_frame_index = 0;
            node->set_animation(active_anim_name);
            preview_frame = 0;
        }

        if (is_selected) {
            ImGui::SetItemDefaultFocus();
        }
    }
    ImGui::EndChild();
}

void SpriteFramesPanel::draw_frames_view(AnimatedSprite2D* node, SpriteFrames* sf) {
    ImGui::Text("Animation Frames (%s):", active_anim_name.c_str());
    ImGui::Separator();

    int total_frames = sf->get_frame_count(active_anim_name);

    // Row 1: Playback Controls
    if (ImGui::Button("|<##FirstFrame", ImVec2(24, 24))) {
        active_frame_index = 0;
        node->set_frame(0);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Jump to First Frame");

    ImGui::SameLine();
    if (ImGui::Button("<##PrevFrame", ImVec2(24, 24))) {
        if (total_frames > 0) {
            active_frame_index = (active_frame_index - 1 + total_frames) % total_frames;
            node->set_frame(active_frame_index);
        }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Step Back 1 Frame");

    ImGui::SameLine();
    bool is_playing = preview_playing || node->is_playing();
    if (is_playing) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
    }
    if (ImGui::Button(is_playing ? "Stop##PreviewPlay" : "Play##PreviewPlay", ImVec2(50, 24))) {
        preview_playing = !preview_playing;
        if (preview_playing) {
            node->play(active_anim_name);
        } else {
            node->pause();
        }
    }
    if (is_playing) {
        ImGui::PopStyleColor();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Animation Preview Playback");

    ImGui::SameLine();
    if (ImGui::Button(">##NextFrame", ImVec2(24, 24))) {
        if (total_frames > 0) {
            active_frame_index = (active_frame_index + 1) % total_frames;
            node->set_frame(active_frame_index);
        }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Step Forward 1 Frame");

    ImGui::SameLine();
    if (ImGui::Button(">|##LastFrame", ImVec2(24, 24))) {
        if (total_frames > 0) {
            active_frame_index = total_frames - 1;
            node->set_frame(active_frame_index);
        }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Jump to Last Frame");

    ImGui::SameLine();
    ImGui::TextDisabled("  Frame: %d / %d", total_frames > 0 ? (active_frame_index + 1) : 0, total_frames);

    // Row 2: Frame Management Toolbar
    if (ImGui::Button("+ File##AddFrameFile")) {
        add_file_buf[0] = '\0';
        show_add_file_popup = true;
        ImGui::OpenPopup("Add Frame from File##Popup");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add Single Frame from Image File");

    ImGui::SameLine();
    if (ImGui::Button("+ Grid / Sheet##AddFrameSheet")) {
        const AnimationFrame* f = sf->get_frame(active_anim_name, active_frame_index);
        std::string def_p = f ? f->texture_path : "";
        open_sheet_modal(def_p);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add Frames from Sprite Sheet (Grid Slicer)");

    ImGui::SameLine();
    if (ImGui::Button("Copy##CopyFrame")) {
        const AnimationFrame* f = sf->get_frame(active_anim_name, active_frame_index);
        if (f) {
            copied_frame_buffer = *f;
            has_copied_frame = true;
        }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Copy Selected Frame");

    ImGui::SameLine();
    if (ImGui::Button("Paste##PasteFrame") && has_copied_frame) {
        sf->add_frame(active_anim_name, copied_frame_buffer.texture_path, copied_frame_buffer.region_rect, copied_frame_buffer.duration);
        active_frame_index = sf->get_frame_count(active_anim_name) - 1;
        EditorState::get()->push_undo_snapshot();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Paste Copied Frame");

    ImGui::SameLine();
    if (ImGui::Button("<-##MoveFrameLeft")) {
        if (active_frame_index > 0 && total_frames > 1) {
            sf->move_frame(active_anim_name, active_frame_index, active_frame_index - 1);
            active_frame_index--;
            EditorState::get()->push_undo_snapshot();
        }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Move Frame Left");

    ImGui::SameLine();
    if (ImGui::Button("->##MoveFrameRight")) {
        if (active_frame_index >= 0 && active_frame_index < total_frames - 1) {
            sf->move_frame(active_anim_name, active_frame_index, active_frame_index + 1);
            active_frame_index++;
            EditorState::get()->push_undo_snapshot();
        }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Move Frame Right");

    ImGui::SameLine();
    if (ImGui::Button("Trash##DelFrame")) {
        if (active_frame_index >= 0 && active_frame_index < total_frames) {
            sf->remove_frame(active_anim_name, active_frame_index);
            if (active_frame_index >= sf->get_frame_count(active_anim_name)) {
                active_frame_index = std::max(0, sf->get_frame_count(active_anim_name) - 1);
            }
            EditorState::get()->push_undo_snapshot();
        }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Delete Selected Frame");

    ImGui::SameLine();
    float dur = sf->get_frame_duration(active_anim_name, active_frame_index);
    ImGui::SetNextItemWidth(70.0f);
    if (ImGui::DragFloat("##Duration", &dur, 0.1f, 0.1f, 10.0f, "x %.1f")) {
        sf->set_frame_duration(active_anim_name, active_frame_index, dur);
        EditorState::get()->push_undo_snapshot();
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Frame Duration Multiplier");

    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0f);
    ImGui::SliderFloat("##Zoom", &thumbnail_size, 32.0f, 128.0f, "Zoom: %.0f");

    // Modal Popup: Add Frame from File
    if (ImGui::BeginPopupModal("Add Frame from File##Popup", &show_add_file_popup, ImGuiWindowFlags_AlwaysAutoResize)) {
        FileDialog::draw_path_picker_buf("Texture Path", add_file_buf, sizeof(add_file_buf), "*.png;*.jpg;*.jpeg;*.bmp");
        if (ImGui::Button("Add", ImVec2(80, 0))) {
            if (add_file_buf[0] != '\0') {
                sf->add_frame(active_anim_name, add_file_buf);
                active_frame_index = sf->get_frame_count(active_anim_name) - 1;
                EditorState::get()->push_undo_snapshot();
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(80, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Scrollable Thumbnails View Area
    ImGui::BeginChild("FramesThumbnailsChild", ImVec2(0, 0), true);

    if (total_frames == 0) {
        ImGui::TextDisabled("No frames in this animation. Click '+ Grid / Sheet' or '+ File' above to add frames.");
    } else {
        float avail_w = ImGui::GetContentRegionAvail().x;
        float item_w = thumbnail_size + 16.0f;
        int items_per_row = std::max(1, static_cast<int>(avail_w / item_w));

        for (int i = 0; i < total_frames; ++i) {
            if (i > 0 && (i % items_per_row) != 0) {
                ImGui::SameLine();
            }

            ImGui::PushID(i);
            const AnimationFrame* f = sf->get_frame(active_anim_name, i);
            bool is_sel = (i == active_frame_index);

            ImGui::BeginGroup();

            SDL_Texture* sdl_tex = nullptr;
            if (f && !f->texture_path.empty()) {
                uint32_t tid = f->texture_id;
                if (tid == 0) {
                    tid = TextureServer::get()->load_texture(f->texture_path);
                }
                sdl_tex = TextureServer::get()->get_texture(tid);
            }

            ImVec4 border_col = is_sel ? ImVec4(0.3f, 0.75f, 1.0f, 1.0f) : ImVec4(0.2f, 0.2f, 0.25f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Border, border_col);

            ImVec2 uv0(0.0f, 0.0f);
            ImVec2 uv1(1.0f, 1.0f);

            if (sdl_tex && f && f->region_rect.size.x > Fixed16(0) && f->region_rect.size.y > Fixed16(0)) {
                Vector2Fixed tex_sz = TextureServer::get()->get_texture_size(f->texture_id);
                if (tex_sz.x > Fixed16(0) && tex_sz.y > Fixed16(0)) {
                    float tw = tex_sz.x.to_float();
                    float th = tex_sz.y.to_float();
                    uv0 = ImVec2(f->region_rect.position.x.to_float() / tw, f->region_rect.position.y.to_float() / th);
                    uv1 = ImVec2((f->region_rect.position.x.to_float() + f->region_rect.size.x.to_float()) / tw,
                                 (f->region_rect.position.y.to_float() + f->region_rect.size.y.to_float()) / th);
                }
            }

            if (sdl_tex) {
                ImGui::ImageButton("##FrameImg", (ImTextureID)sdl_tex, ImVec2(thumbnail_size, thumbnail_size), uv0, uv1, ImVec4(0,0,0,0), is_sel ? ImVec4(0.3f, 0.75f, 1.0f, 1.0f) : ImVec4(1,1,1,1));
            } else {
                ImGui::Button("##NoImg", ImVec2(thumbnail_size, thumbnail_size));
            }

            if (ImGui::IsItemClicked()) {
                active_frame_index = i;
                node->set_frame(i);
            }

            ImGui::PopStyleColor();

            // Frame Label under Thumbnail
            float dur_factor = f ? f->duration : 1.0f;
            if (dur_factor != 1.0f) {
                ImGui::Text(" %d (x%.1f)", i, dur_factor);
            } else {
                ImGui::Text("    %d", i);
            }

            ImGui::EndGroup();
            ImGui::PopID();
        }
    }

    ImGui::EndChild();
}

void SpriteFramesPanel::draw_sprite_sheet_modal(AnimatedSprite2D* node, SpriteFrames* sf) {
    ImGui::SetNextWindowSize(ImVec2(800, 520), ImGuiCond_FirstUseEver);
    if (!ImGui::BeginPopupModal("Add Frames from Sprite Sheet##Modal", &show_sheet_modal, ImGuiWindowFlags_None)) {
        ImGui::OpenPopup("Add Frames from Sprite Sheet##Modal");
        return;
    }

    ImGui::TextUnformatted("Configure Sprite Sheet Grid Slicer:");
    ImGui::Separator();

    // Texture path selector
    if (FileDialog::draw_path_picker("Texture Path", sheet_texture_path, "*.png;*.jpg;*.jpeg;*.bmp", 460.0f)) {
        cell_selection.clear();
    }

    uint32_t tex_id = 0;
    SDL_Texture* tex = nullptr;
    Vector2Fixed tex_size = Vector2Fixed::zero();
    if (!sheet_texture_path.empty()) {
        tex_id = TextureServer::get()->load_texture(sheet_texture_path);
        tex = TextureServer::get()->get_texture(tex_id);
        tex_size = TextureServer::get()->get_texture_size(tex_id);
    }

    int img_w = static_cast<int>(tex_size.x.to_float());
    int img_h = static_cast<int>(tex_size.y.to_float());

    ImGui::Columns(2, "SheetModalColumns", true);
    static bool set_modal_col_width = false;
    if (!set_modal_col_width) {
        ImGui::SetColumnWidth(0, 320.0f);
        set_modal_col_width = true;
    }

    // Left Column: Slicer Controls
    ImGui::Text("Texture Resolution: %dx%d px", img_w, img_h);
    ImGui::RadioButton("By Frame Count (Cols / Rows)", &slice_mode, 0);
    ImGui::RadioButton("By Frame Size (Width / Height)", &slice_mode, 1);

    if (slice_mode == 0) {
        // By Frame Count
        if (ImGui::DragInt("Columns (H)", &sheet_cols, 1.0f, 1, 64)) {
            cell_selection.clear();
        }
        if (ImGui::DragInt("Rows (V)", &sheet_rows, 1.0f, 1, 64)) {
            cell_selection.clear();
        }

        if (img_w > 0 && sheet_cols > 0) frame_width = (img_w - offset_x - (sheet_cols - 1) * separation_x) / sheet_cols;
        if (img_h > 0 && sheet_rows > 0) frame_height = (img_h - offset_y - (sheet_rows - 1) * separation_y) / sheet_rows;

        ImGui::TextDisabled("Calculated Frame Size: %dx%d px", frame_width, frame_height);
    } else {
        // By Frame Size
        if (ImGui::DragInt("Frame Width (px)", &frame_width, 1.0f, 1, 1024)) {
            cell_selection.clear();
        }
        if (ImGui::DragInt("Frame Height (px)", &frame_height, 1.0f, 1, 1024)) {
            cell_selection.clear();
        }

        if (frame_width > 0) sheet_cols = std::max(1, (img_w - offset_x + separation_x) / (frame_width + separation_x));
        if (frame_height > 0) sheet_rows = std::max(1, (img_h - offset_y + separation_y) / (frame_height + separation_y));

        ImGui::TextDisabled("Calculated Grid: %dx%d (Cols x Rows)", sheet_cols, sheet_rows);
    }

    ImGui::Separator();
    if (ImGui::DragInt("Separation X (px)", &separation_x, 1.0f, 0, 128)) cell_selection.clear();
    if (ImGui::DragInt("Separation Y (px)", &separation_y, 1.0f, 0, 128)) cell_selection.clear();
    if (ImGui::DragInt("Offset X (px)", &offset_x, 1.0f, 0, 512)) cell_selection.clear();
    if (ImGui::DragInt("Offset Y (px)", &offset_y, 1.0f, 0, 512)) cell_selection.clear();

    int total_cells = std::max(0, sheet_cols * sheet_rows);
    if (static_cast<int>(cell_selection.size()) != total_cells) {
        cell_selection.assign(total_cells, true); // Select all by default
    }

    int selected_count = 0;
    for (bool b : cell_selection) {
        if (b) selected_count++;
    }

    ImGui::Separator();
    ImGui::Text("Selection: %d / %d cells", selected_count, total_cells);

    if (ImGui::Button("Select All")) {
        cell_selection.assign(total_cells, true);
    }
    ImGui::SameLine();
    if (ImGui::Button("Deselect All")) {
        cell_selection.assign(total_cells, false);
    }
    ImGui::SameLine();
    if (ImGui::Button("Invert")) {
        for (size_t i = 0; i < cell_selection.size(); ++i) {
            cell_selection[i] = !cell_selection[i];
        }
    }

    ImGui::NextColumn();

    // Right Column: Interactive Texture Grid Canvas
    ImGui::TextUnformatted("Interactive Sheet Grid Preview (Click cell to toggle selection):");
    ImGui::BeginChild("SheetPreviewCanvas", ImVec2(0, 320), true, ImGuiWindowFlags_HorizontalScrollbar);

    if (tex && img_w > 0 && img_h > 0) {
        ImVec2 canvas_p = ImGui::GetCursorScreenPos();
        float display_w = static_cast<float>(img_w);
        float display_h = static_cast<float>(img_h);

        ImGui::Image((ImTextureID)tex, ImVec2(display_w, display_h));

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Render Grid Overlay & Cells
        int idx = 0;
        for (int r = 0; r < sheet_rows; ++r) {
            for (int c = 0; c < sheet_cols; ++c) {
                if (idx >= total_cells) break;

                float rx = canvas_p.x + offset_x + c * (frame_width + separation_x);
                float ry = canvas_p.y + offset_y + r * (frame_height + separation_y);
                float rw = static_cast<float>(frame_width);
                float rh = static_cast<float>(frame_height);

                ImVec2 p_min(rx, ry);
                ImVec2 p_max(rx + rw, ry + rh);

                bool is_sel = cell_selection[idx];

                if (is_sel) {
                    draw_list->AddRectFilled(p_min, p_max, IM_COL32(50, 200, 80, 100));
                    draw_list->AddRect(p_min, p_max, IM_COL32(80, 255, 120, 255), 0.0f, 0, 2.0f);
                } else {
                    draw_list->AddRect(p_min, p_max, IM_COL32(255, 255, 255, 120), 0.0f, 0, 1.0f);
                }

                // Cell Index Label
                char num_str[16];
                snprintf(num_str, sizeof(num_str), "%d", idx);
                draw_list->AddText(ImVec2(rx + 2.0f, ry + 2.0f), IM_COL32(255, 255, 255, 220), num_str);

                idx++;
            }
        }

        // Handle Click Selection on Canvas
        if (ImGui::IsItemClicked()) {
            ImVec2 mouse_pos = ImGui::GetMousePos();
            float rel_x = mouse_pos.x - canvas_p.x - offset_x;
            float rel_y = mouse_pos.y - canvas_p.y - offset_y;

            if (rel_x >= 0 && rel_y >= 0) {
                int col = static_cast<int>(rel_x) / (frame_width + separation_x);
                int row = static_cast<int>(rel_y) / (frame_height + separation_y);

                if (col >= 0 && col < sheet_cols && row >= 0 && row < sheet_rows) {
                    int clicked_idx = row * sheet_cols + col;
                    if (clicked_idx >= 0 && clicked_idx < total_cells) {
                        cell_selection[clicked_idx] = !cell_selection[clicked_idx];
                    }
                }
            }
        }
    } else {
        ImGui::TextDisabled("Specify a valid texture path above to preview the sprite sheet grid.");
    }

    ImGui::EndChild();

    ImGui::Columns(1);
    ImGui::Separator();

    // Bottom Action Buttons
    std::string btn_label = "Add " + std::to_string(selected_count) + " Frames to '" + active_anim_name + "'";
    if (ImGui::Button(btn_label.c_str(), ImVec2(240, 32))) {
        if (selected_count > 0 && !sheet_texture_path.empty() && frame_width > 0 && frame_height > 0) {
            int cell_i = 0;
            for (int r = 0; r < sheet_rows; ++r) {
                for (int c = 0; c < sheet_cols; ++c) {
                    if (cell_i >= total_cells) break;

                    if (cell_selection[cell_i]) {
                        float rx = static_cast<float>(offset_x + c * (frame_width + separation_x));
                        float ry = static_cast<float>(offset_y + r * (frame_height + separation_y));
                        float rw = static_cast<float>(frame_width);
                        float rh = static_cast<float>(frame_height);

                        Rect2Fixed region = Rect2Fixed::from_floats(rx, ry, rw, rh);
                        sf->add_frame(active_anim_name, sheet_texture_path, region);
                    }
                    cell_i++;
                }
            }

            active_frame_index = sf->get_frame_count(active_anim_name) - 1;
            node->set_animation(active_anim_name);
            EditorState::get()->push_undo_snapshot();
            show_sheet_modal = false;
            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(100, 32))) {
        show_sheet_modal = false;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

} // namespace RetroNode
