#include "filesystem_panel.h"
#include "../editor_state.h"
#include <imgui.h>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace RetroNode {

void FileSystemPanel::draw_directory_contents(const std::string& dir_path) {
    std::error_code ec;
    if (!fs::exists(dir_path, ec) || !fs::is_directory(dir_path, ec)) return;

    for (const auto& entry : fs::directory_iterator(dir_path, ec)) {
        std::string filename = entry.path().filename().string();
        if (filename.length() > 0 && filename[0] == '.') continue; // Skip hidden

        std::string full_path = entry.path().string();

        if (entry.is_directory(ec)) {
            if (ImGui::TreeNode(filename.c_str())) {
                draw_directory_contents(full_path);
                ImGui::TreePop();
            }
        } else {
            ImGui::Selectable(filename.c_str());

            // Compute relative "res://" path
            std::string proj_dir = EditorState::get()->get_project_dir();
            std::string res_path = full_path;
            if (res_path.rfind(proj_dir, 0) == 0) {
                res_path = "res://" + res_path.substr(proj_dir.length() + 1);
            }
            // Normalize backslashes to forward slashes
            for (auto& ch : res_path) {
                if (ch == '\\') ch = '/';
            }

            // Drag and Drop Source for Asset Paths
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                ImGui::SetDragDropPayload("RN_ASSET_PATH", res_path.c_str(), res_path.length() + 1);
                ImGui::Text("Asset: %s", filename.c_str());
                ImGui::EndDragDropSource();
            }

            // Double Click to Open Scene Files
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                std::string ext = entry.path().extension().string();
                if (ext == ".json" || ext == ".rnb") {
                    EditorState::get()->open_scene(full_path);
                }
            }
        }
    }
}

void FileSystemPanel::draw() {
    ImGui::Begin("FileSystem");

    std::string proj_dir = EditorState::get()->get_project_dir();
    ImGui::TextDisabled("Project: %s", proj_dir.c_str());
    ImGui::Separator();

    draw_directory_contents(proj_dir);

    ImGui::End();
}

} // namespace RetroNode
