#include "file_dialog.h"
#include "editor_state.h"
#include "../servers/texture_server.h"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <iostream>
#include <algorithm>

namespace fs = std::filesystem;

namespace RetroNode {

FileDialog* FileDialog::instance = nullptr;

FileDialog::FileDialog() {
    std::string proj_dir = EditorState::get()->get_project_dir();
    current_dir = proj_dir.empty() ? "." : proj_dir;
    dir_history.push_back(current_dir);
}

std::string FileDialog::normalize_path(const std::string& full_path) {
    if (full_path.empty()) return "";
    std::string proj_dir = EditorState::get()->get_project_dir();
    std::string norm_proj = proj_dir;
    for (auto& c : norm_proj) if (c == '\\') c = '/';
    if (!norm_proj.empty() && norm_proj.back() == '/') norm_proj.pop_back();

    std::string norm_file = full_path;
    for (auto& c : norm_file) if (c == '\\') c = '/';

    if (norm_file.rfind("res://", 0) == 0) {
        return norm_file;
    }

    if (!norm_proj.empty() && norm_file.rfind(norm_proj, 0) == 0) {
        std::string rel = norm_file.substr(norm_proj.length());
        if (!rel.empty() && rel[0] == '/') rel = rel.substr(1);
        return "res://" + rel;
    }

    return norm_file;
}

void FileDialog::navigate_to(const std::string& path) {
    std::error_code ec;
    if (fs::exists(path, ec) && fs::is_directory(path, ec)) {
        current_dir = path;
        if (history_index + 1 < dir_history.size()) {
            dir_history.resize(history_index + 1);
        }
        dir_history.push_back(current_dir);
        history_index = dir_history.size() - 1;
    }
}

void FileDialog::open(
    const std::string& title,
    const std::string& filter,
    const std::string& initial_path,
    std::function<void(const std::string&)> callback
) {
    dialog_title = title.empty() ? "Select File" : title;
    filter_pattern = filter.empty() ? "*.*" : filter;
    on_select_callback = callback;
    selected_file_path = initial_path;

    std::string proj_dir = EditorState::get()->get_project_dir();
    std::string start_dir = proj_dir.empty() ? "." : proj_dir;

    if (!initial_path.empty()) {
        std::string resolved = initial_path;
        if (resolved.rfind("res://", 0) == 0) {
            resolved = proj_dir + "/" + resolved.substr(6);
        }
        std::error_code ec;
        fs::path p(resolved);
        if (fs::exists(p, ec)) {
            if (fs::is_directory(p, ec)) {
                start_dir = p.string();
            } else if (p.has_parent_path()) {
                start_dir = p.parent_path().string();
            }
        }
    }

    current_dir = start_dir;
    is_open = true;
    ImGui::OpenPopup((dialog_title + "##FileDialogModal").c_str());
}

static bool matches_filter(const std::string& filename, const std::string& pattern) {
    if (pattern == "*.*" || pattern == "*" || pattern.empty()) return true;

    std::string lower_file = filename;
    std::transform(lower_file.begin(), lower_file.end(), lower_file.begin(), [](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });

    std::string lower_pattern = pattern;
    std::transform(lower_pattern.begin(), lower_pattern.end(), lower_pattern.begin(), [](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });

    // Handle multi-patterns separated by semicolon (e.g. "*.png;*.jpg;*.jpeg")
    size_t start = 0;
    while (start < lower_pattern.length()) {
        size_t end = lower_pattern.find(';', start);
        if (end == std::string::npos) end = lower_pattern.length();

        std::string sub = lower_pattern.substr(start, end - start);
        if (!sub.empty()) {
            if (sub.rfind("*.", 0) == 0) {
                std::string ext = sub.substr(1); // e.g. ".png"
                if (lower_file.length() >= ext.length() && lower_file.substr(lower_file.length() - ext.length()) == ext) {
                    return true;
                }
            } else if (sub == lower_file) {
                return true;
            }
        }
        start = end + 1;
    }
    return false;
}

static void draw_directory_tree_recursive(const std::string& dir_path, std::string& current_dir) {
    std::error_code ec;
    if (!fs::exists(dir_path, ec) || !fs::is_directory(dir_path, ec)) return;

    for (const auto& entry : fs::directory_iterator(dir_path, ec)) {
        if (!entry.is_directory(ec)) continue;

        std::string dirname = entry.path().filename().string();
        if (dirname.length() > 0 && dirname[0] == '.') continue; // Skip hidden

        std::string full_path = entry.path().string();
        bool is_selected = (full_path == current_dir);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if (is_selected) flags |= ImGuiTreeNodeFlags_Selected;

        bool open = ImGui::TreeNodeEx(dirname.c_str(), flags);
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            current_dir = full_path;
        }

        if (open) {
            draw_directory_tree_recursive(full_path, current_dir);
            ImGui::TreePop();
        }
    }
}

void FileDialog::draw() {
    if (!is_open) return;

    ImGui::SetNextWindowSize(ImVec2(780, 480), ImGuiCond_FirstUseEver);
    std::string popup_id = dialog_title + "##FileDialogModal";

    if (!ImGui::BeginPopupModal(popup_id.c_str(), &is_open, ImGuiWindowFlags_None)) {
        return;
    }

    std::string proj_dir = EditorState::get()->get_project_dir();

    // Top Navigation & Breadcrumb Bar
    if (ImGui::Button("<##Back")) {
        if (history_index > 0) {
            history_index--;
            current_dir = dir_history[history_index];
        }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Back");

    ImGui::SameLine();
    if (ImGui::Button("^##Up")) {
        std::error_code ec;
        fs::path p(current_dir);
        if (p.has_parent_path() && fs::exists(p.parent_path(), ec)) {
            navigate_to(p.parent_path().string());
        }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Parent Directory");

    ImGui::SameLine();
    if (ImGui::Button("Home##Home")) {
        navigate_to(proj_dir);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Project Root");

    // Breadcrumb path display
    ImGui::SameLine();
    std::string display_path = normalize_path(current_dir);
    ImGui::TextColored(ImVec4(0.3f, 0.75f, 1.0f, 1.0f), " %s", display_path.c_str());

    // Search and Filter
    ImGui::SameLine(ImGui::GetWindowWidth() - 250.0f);
    ImGui::SetNextItemWidth(240.0f);
    ImGui::InputTextWithHint("##FileSearch", "Search files...", search_buf, sizeof(search_buf));

    ImGui::Separator();

    // Main 2-column Body
    ImGui::Columns(2, "FileDialogColumns", true);
    static bool set_col_w = false;
    if (!set_col_w) {
        ImGui::SetColumnWidth(0, 220.0f);
        set_col_w = true;
    }

    // Left Column: Directory Tree
    ImGui::TextUnformatted("Folders:");
    ImGui::BeginChild("DirTreeChild", ImVec2(0, 320), true);

    std::string root_label = "res:// (" + fs::path(proj_dir).filename().string() + ")";
    bool root_selected = (current_dir == proj_dir);
    ImGuiTreeNodeFlags root_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
    if (root_selected) root_flags |= ImGuiTreeNodeFlags_Selected;

    if (ImGui::TreeNodeEx(root_label.c_str(), root_flags)) {
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            current_dir = proj_dir;
        }
        draw_directory_tree_recursive(proj_dir, current_dir);
        ImGui::TreePop();
    }

    ImGui::EndChild();

    ImGui::NextColumn();

    // Right Column: Files View
    ImGui::TextUnformatted("Files:");
    ImGui::BeginChild("FilesListChild", ImVec2(0, 320), true);

    std::error_code ec;
    if (fs::exists(current_dir, ec) && fs::is_directory(current_dir, ec)) {
        std::string search_str = search_buf;
        std::transform(search_str.begin(), search_str.end(), search_str.begin(), [](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });

        for (const auto& entry : fs::directory_iterator(current_dir, ec)) {
            std::string fname = entry.path().filename().string();
            if (fname.length() > 0 && fname[0] == '.') continue;

            std::string full_path = entry.path().string();

            if (entry.is_directory(ec)) {
                std::string folder_label = "[Dir] " + fname;
                if (ImGui::Selectable(folder_label.c_str())) {
                    navigate_to(full_path);
                    break;
                }
            } else {
                if (!matches_filter(fname, filter_pattern)) continue;

                if (!search_str.empty()) {
                    std::string lower_fname = fname;
                    std::transform(lower_fname.begin(), lower_fname.end(), lower_fname.begin(), [](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });
                    if (lower_fname.find(search_str) == std::string::npos) continue;
                }

                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });

                std::string icon = "  ";
                if (ext == ".png" || ext == ".jpg" || ext == ".bmp") icon = "[IMG] ";
                else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg") icon = "[AUDIO] ";
                else if (ext == ".json" || ext == ".rnb") icon = "[SCENE] ";

                std::string item_label = icon + fname;
                bool is_sel = (selected_file_path == normalize_path(full_path));

                if (ImGui::Selectable(item_label.c_str(), is_sel)) {
                    selected_file_path = normalize_path(full_path);
                }

                // Image Hover Preview Tooltip
                if (ImGui::IsItemHovered() && (ext == ".png" || ext == ".jpg" || ext == ".bmp")) {
                    uint32_t tid = TextureServer::get()->load_texture(normalize_path(full_path));
                    SDL_Texture* tex = TextureServer::get()->get_texture(tid);
                    if (tex) {
                        Vector2Fixed sz = TextureServer::get()->get_texture_size(tid);
                        ImGui::BeginTooltip();
                        ImGui::Text("%s (%dx%d px)", fname.c_str(), static_cast<int>(sz.x.to_float()), static_cast<int>(sz.y.to_float()));
                        ImGui::Image((ImTextureID)tex, ImVec2(128, 128));
                        ImGui::EndTooltip();
                    }
                }

                // Double Click to Select File
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    selected_file_path = normalize_path(full_path);
                    if (on_select_callback) {
                        on_select_callback(selected_file_path);
                    }
                    is_open = false;
                    ImGui::CloseCurrentPopup();
                    break;
                }
            }
        }
    }

    ImGui::EndChild();

    ImGui::Columns(1);
    ImGui::Separator();

    // Bottom Selected File Box & Action Buttons
    char sel_buf[256];
    snprintf(sel_buf, sizeof(sel_buf), "%s", selected_file_path.c_str());
    ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - 200.0f);
    if (ImGui::InputText("##SelectedPath", sel_buf, sizeof(sel_buf))) {
        selected_file_path = sel_buf;
    }

    ImGui::SameLine();
    if (ImGui::Button("Select", ImVec2(80, 0))) {
        if (!selected_file_path.empty()) {
            if (on_select_callback) {
                on_select_callback(selected_file_path);
            }
        }
        is_open = false;
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(80, 0))) {
        is_open = false;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

bool FileDialog::draw_path_picker(
    const char* label,
    std::string& path_value,
    const char* filter,
    float width
) {
    ImGui::PushID(label);
    bool value_changed = false;

    if (width > 0.0f) {
        ImGui::SetNextItemWidth(width);
    }

    char buf[256];
    snprintf(buf, sizeof(buf), "%s", path_value.c_str());

    if (ImGui::InputText(label, buf, sizeof(buf))) {
        path_value = buf;
        value_changed = true;
    }

    // Drag and Drop target
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RN_ASSET_PATH")) {
            const char* asset_path = (const char*)payload->Data;
            path_value = asset_path;
            value_changed = true;
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine();
    std::string btn_id = "Browse##" + std::string(label);
    if (ImGui::Button(btn_id.c_str())) {
        std::string filter_str = filter ? filter : "*.*";
        std::string current_val = path_value;
        FileDialog::get()->open("Select " + std::string(label), filter_str, current_val, [&path_value](const std::string& new_path) {
            path_value = new_path;
            EditorState::get()->push_undo_snapshot();
        });
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Explore & Select File...");

    ImGui::PopID();
    return value_changed;
}

bool FileDialog::draw_path_picker_buf(
    const char* label,
    char* buffer,
    size_t buffer_size,
    const char* filter,
    float width
) {
    std::string val = buffer;
    bool changed = draw_path_picker(label, val, filter, width);
    if (changed) {
        snprintf(buffer, buffer_size, "%s", val.c_str());
    }
    return changed;
}

} // namespace RetroNode
