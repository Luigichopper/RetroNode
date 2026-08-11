#include "project_manager.h"
#include "editor_state.h"
#include "../servers/visual_server.h"
#include "../servers/texture_server.h"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <ctime>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace RetroNode {

ProjectManager* ProjectManager::instance = nullptr;

ProjectManager::ProjectManager() {
    init();
}

ProjectManager::~ProjectManager() {
    save_projects_json();
}

std::string ProjectManager::get_config_filepath() const {
    return "projects.json";
}

void ProjectManager::init() {
    load_projects_json();

    // If no projects exist in projects.json, auto-scan working directory for project.rnode
    if (projects.empty()) {
        scan_directory(".");
        if (projects.empty() && fs::exists("./MyRPG")) {
            add_project("./MyRPG");
        }
    }
}

void ProjectManager::load_projects_json() {
    projects.clear();
    std::string config_path = get_config_filepath();
    if (!fs::exists(config_path)) return;

    try {
        std::ifstream file(config_path);
        if (file.is_open()) {
            json j;
            file >> j;
            if (j.contains("projects") && j["projects"].is_array()) {
                for (const auto& item : j["projects"]) {
                    ProjectInfo p;
                    p.name = item.value("name", "Untitled Project");
                    p.path = item.value("path", "");
                    p.main_scene = item.value("main_scene", "scenes/main.json");
                    p.icon_path = item.value("icon_path", "");
                    p.is_favorite = item.value("is_favorite", false);
                    p.last_modified = item.value("last_modified", "Unknown");
                    p.engine_version = item.value("engine_version", "v0.1.0");

                    if (item.contains("tags") && item["tags"].is_array()) {
                        for (const auto& tag : item["tags"]) {
                            if (tag.is_string()) p.tags.push_back(tag.get<std::string>());
                        }
                    }

                    // Check liveness on disk
                    std::error_code ec;
                    p.is_missing = !fs::exists(p.path, ec) || !fs::is_directory(p.path, ec);

                    if (!p.path.empty()) {
                        projects.push_back(p);
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[ProjectManager] Error reading projects.json: " << e.what() << std::endl;
    }
}

void ProjectManager::save_projects_json() {
    try {
        json j;
        json proj_list = json::array();

        for (const auto& p : projects) {
            json item;
            item["name"] = p.name;
            item["path"] = p.path;
            item["main_scene"] = p.main_scene;
            item["icon_path"] = p.icon_path;
            item["is_favorite"] = p.is_favorite;
            item["last_modified"] = p.last_modified;
            item["engine_version"] = p.engine_version;
            item["tags"] = p.tags;
            proj_list.push_back(item);
        }

        j["projects"] = proj_list;

        std::ofstream file(get_config_filepath());
        if (file.is_open()) {
            file << j.dump(4);
            std::cout << "[ProjectManager] Saved " << projects.size() << " project entries to projects.json" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[ProjectManager] Error saving projects.json: " << e.what() << std::endl;
    }
}

static std::string get_current_time_string() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    char buf[64];
    struct tm tm_buf;
#if defined(_WIN32)
    localtime_s(&tm_buf, &now_time);
#else
    localtime_r(&now_time, &tm_buf);
#endif
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm_buf);
    return std::string(buf);
}

bool ProjectManager::add_project(const std::string& project_dir_path) {
    std::error_code ec;
    std::string canonical_path = fs::weakly_canonical(project_dir_path, ec).string();
    if (canonical_path.empty()) canonical_path = project_dir_path;

    // Check if already registered
    for (auto& existing : projects) {
        if (existing.path == canonical_path || existing.path == project_dir_path) {
            existing.is_missing = false;
            status_message = "Project already in library: " + existing.name;
            return true;
        }
    }

    ProjectInfo p;
    p.path = canonical_path;
    p.name = fs::path(canonical_path).filename().string();
    if (p.name.empty()) p.name = "New Project";
    p.last_modified = get_current_time_string();
    p.engine_version = "v0.1.0";
    p.is_missing = !fs::exists(canonical_path, ec);

    // Read project.rnode if present
    std::string rnode_file = canonical_path + "/project.rnode";
    if (fs::exists(rnode_file, ec)) {
        try {
            std::ifstream file(rnode_file);
            if (file.is_open()) {
                json j;
                file >> j;
                if (j.contains("name") && j["name"].is_string()) p.name = j["name"];
                if (j.contains("main_scene") && j["main_scene"].is_string()) p.main_scene = j["main_scene"];
            }
        } catch (...) {}
    }

    projects.push_back(p);
    save_projects_json();
    status_message = "Imported project: " + p.name;
    return true;
}

void ProjectManager::scan_directory(const std::string& root_dir) {
    std::error_code ec;
    if (!fs::exists(root_dir, ec) || !fs::is_directory(root_dir, ec)) return;

    int added_count = 0;
    for (const auto& entry : fs::directory_iterator(root_dir, ec)) {
        if (entry.is_directory(ec)) {
            std::string sub_path = entry.path().string();
            if (fs::exists(sub_path + "/project.rnode", ec)) {
                if (add_project(sub_path)) {
                    added_count++;
                }
            }
        }
    }
    status_message = "Scanned directory: added " + std::to_string(added_count) + " projects.";
}

void ProjectManager::remove_project(int index, bool delete_files_from_disk) {
    if (index < 0 || index >= static_cast<int>(projects.size())) return;

    std::string name = projects[index].name;
    std::string path = projects[index].path;

    if (delete_files_from_disk && !path.empty()) {
        std::error_code ec;
        fs::remove_all(path, ec);
    }

    projects.erase(projects.begin() + index);
    if (selected_project_idx >= static_cast<int>(projects.size())) {
        selected_project_idx = static_cast<int>(projects.size()) - 1;
    }
    save_projects_json();
    status_message = "Removed project: " + name;
}

void ProjectManager::remove_missing_projects() {
    int orig_size = static_cast<int>(projects.size());
    projects.erase(std::remove_if(projects.begin(), projects.end(), [](const ProjectInfo& p) {
        return p.is_missing;
    }), projects.end());

    int removed_count = orig_size - static_cast<int>(projects.size());
    selected_project_idx = -1;
    save_projects_json();
    status_message = "Removed " + std::to_string(removed_count) + " missing project entries.";
}

bool ProjectManager::create_new_project(const std::string& proj_name, const std::string& parent_dir, int template_idx) {
    if (proj_name.empty()) return false;
    std::string target_dir = parent_dir;
    if (target_dir.empty()) target_dir = ".";
    target_dir = target_dir + "/" + proj_name;

    std::error_code ec;
    fs::create_directories(target_dir + "/scenes", ec);
    fs::create_directories(target_dir + "/assets", ec);
    fs::create_directories(target_dir + "/src", ec);

    // Create project.rnode manifest
    json rnode;
    rnode["name"] = proj_name;
    rnode["main_scene"] = "scenes/main.json";
    rnode["display"] = {
        {"window_width", 1024},
        {"window_height", 896},
        {"virtual_width", 256},
        {"virtual_height", 224},
        {"target_fps", 60}
    };
    rnode["input"] = json::object();

    std::ofstream rfile(target_dir + "/project.rnode");
    if (rfile.is_open()) {
        rfile << rnode.dump(4);
    }

    // Create starter scene
    json scene_json;
    scene_json["name"] = "MainScene";
    scene_json["type"] = "Node2D";
    scene_json["properties"] = json::object();

    json child_node;
    if (template_idx == 1) { // 2D Platformer
        child_node["name"] = "Player";
        child_node["type"] = "CharacterBody2D";
        child_node["properties"] = { {"position", {128, 112}} };
    } else if (template_idx == 2) { // Top-Down RPG
        child_node["name"] = "Hero";
        child_node["type"] = "Sprite2D";
        child_node["properties"] = { {"position", {128, 112}} };
    } else if (template_idx == 3) { // Particle Showcase
        child_node["name"] = "Sparkles";
        child_node["type"] = "CPUParticles2D";
        child_node["properties"] = { {"amount", 32}, {"emitting", true} };
    } else {
        child_node["name"] = "Label";
        child_node["type"] = "Label";
        child_node["properties"] = { {"text", "Welcome to " + proj_name + "!"}, {"position", {16, 16}} };
    }

    scene_json["children"] = json::array({ child_node });

    std::ofstream sfile(target_dir + "/scenes/main.json");
    if (sfile.is_open()) {
        sfile << scene_json.dump(4);
    }

    add_project(target_dir);
    status_message = "Successfully created new project: " + proj_name;
    return true;
}

bool ProjectManager::duplicate_project(int index, const std::string& new_name) {
    if (index < 0 || index >= static_cast<int>(projects.size())) return false;

    std::string src_path = projects[index].path;
    std::string parent_dir = fs::path(src_path).parent_path().string();
    std::string dst_path = parent_dir + "/" + new_name;

    std::error_code ec;
    fs::copy(src_path, dst_path, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);

    // Update project.rnode in duplicate
    std::string dst_rnode = dst_path + "/project.rnode";
    if (fs::exists(dst_rnode, ec)) {
        try {
            json j;
            std::ifstream rfile(dst_rnode);
            if (rfile.is_open()) rfile >> j;
            rfile.close();
            j["name"] = new_name;
            std::ofstream wfile(dst_rnode);
            if (wfile.is_open()) wfile << j.dump(4);
        } catch (...) {}
    }

    add_project(dst_path);
    status_message = "Duplicated project to: " + new_name;
    return true;
}

bool ProjectManager::rename_project(int index, const std::string& new_name) {
    if (index < 0 || index >= static_cast<int>(projects.size()) || new_name.empty()) return false;

    projects[index].name = new_name;
    std::string rnode_file = projects[index].path + "/project.rnode";
    std::error_code ec;
    if (fs::exists(rnode_file, ec)) {
        try {
            json j;
            std::ifstream rfile(rnode_file);
            if (rfile.is_open()) rfile >> j;
            rfile.close();
            j["name"] = new_name;
            std::ofstream wfile(rnode_file);
            if (wfile.is_open()) wfile << j.dump(4);
        } catch (...) {}
    }

    save_projects_json();
    status_message = "Renamed project to: " + new_name;
    return true;
}

void ProjectManager::sort_projects() {
    std::sort(projects.begin(), projects.end(), [this](const ProjectInfo& a, const ProjectInfo& b) {
        if (sort_mode == 3) { // Favorites first
            if (a.is_favorite != b.is_favorite) return a.is_favorite > b.is_favorite;
        }
        if (sort_mode == 1) return a.name < b.name;
        if (sort_mode == 2) return a.path < b.path;
        return a.last_modified > b.last_modified; // Default: Last edited
    });
}

void ProjectManager::draw_ui(SDL_Window* window, SDL_Renderer* renderer) {
    (void)window;
    (void)renderer;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));
    ImGui::Begin("ProjectManagerMainWindow", nullptr, flags);

    // Top Header Bar with Pixel Cartridge Logo
    uint32_t logo_tex_id = TextureServer::get()->load_texture("assets/logo_128.png");
    SDL_Texture* logo_tex = TextureServer::get()->get_texture(logo_tex_id);
    if (logo_tex) {
        SDL_SetTextureScaleMode(logo_tex, SDL_SCALEMODE_NEAREST);
        ImGui::Image((ImTextureID)(intptr_t)logo_tex, ImVec2(40.0f, 40.0f));
        ImGui::SameLine();
    }

    ImGui::BeginGroup();
    ImGui::TextColored(ImVec4(0.3f, 0.75f, 1.0f, 1.0f), "RETRO NODE ENGINE");
    ImGui::TextDisabled("Game-Agnostic 2D Fixed-Point Engine — Project Manager");
    ImGui::EndGroup();

    ImGui::SameLine(ImGui::GetWindowWidth() - 240.0f);


    if (ImGui::Button(current_tab == ProjectManagerTab::PROJECTS ? "[ Projects ]" : "  Projects  ")) {
        current_tab = ProjectManagerTab::PROJECTS;
    }
    ImGui::SameLine();
    if (ImGui::Button(current_tab == ProjectManagerTab::ASSET_LIBRARY ? "[ Templates ]" : "  Templates  ")) {
        current_tab = ProjectManagerTab::ASSET_LIBRARY;
    }
    ImGui::SameLine();
    if (ImGui::Button("Settings")) {
        show_settings_modal = true;
    }

    ImGui::Separator();
    ImGui::Spacing();

    if (current_tab == ProjectManagerTab::PROJECTS) {
        // Toolbar (Create, Import, Scan, Filter, Sort)
        if (ImGui::Button("+ Create", ImVec2(90.0f, 28.0f))) {
            show_create_modal = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Import", ImVec2(80.0f, 28.0f))) {
            show_import_modal = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Scan", ImVec2(70.0f, 28.0f))) {
            show_scan_modal = true;
        }

        ImGui::SameLine(ImGui::GetWindowWidth() - 400.0f);
        ImGui::SetNextItemWidth(200.0f);
        ImGui::InputTextWithHint("##Filter", "Filter Projects...", filter_buf, sizeof(filter_buf));

        ImGui::SameLine();
        ImGui::SetNextItemWidth(130.0f);
        const char* sort_items[] = { "Last Edited", "Name", "Path", "Favorites" };
        if (ImGui::Combo("##Sort", &sort_mode, sort_items, 4)) {
            sort_projects();
        }

        ImGui::Spacing();

        // Main Layout Split: Left (Projects List), Right (Actions Panel)
        float sidebar_width = 180.0f;
        float content_height = ImGui::GetContentRegionAvail().y - 28.0f;

        ImGui::BeginChild("ProjectsListRegion", ImVec2(ImGui::GetContentRegionAvail().x - sidebar_width - 12.0f, content_height), true);

        std::string filter_str = filter_buf;
        std::transform(filter_str.begin(), filter_str.end(), filter_str.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        for (int i = 0; i < static_cast<int>(projects.size()); ++i) {
            auto& proj = projects[i];

            if (!filter_str.empty()) {
                std::string n_lower = proj.name;
                std::string p_lower = proj.path;
                std::transform(n_lower.begin(), n_lower.end(), n_lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                std::transform(p_lower.begin(), p_lower.end(), p_lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                if (n_lower.find(filter_str) == std::string::npos && p_lower.find(filter_str) == std::string::npos) {
                    continue;
                }
            }

            ImGui::PushID(i);

            // Favorite Star Button
            if (ImGui::Selectable(proj.is_favorite ? "[★]##fav" : "[☆]##fav", false, 0, ImVec2(32.0f, 42.0f))) {
                proj.is_favorite = !proj.is_favorite;
                sort_projects();
                save_projects_json();
            }
            ImGui::SameLine();

            bool is_selected = (selected_project_idx == i);

            // Selectable Card Region
            ImGui::BeginGroup();
            if (proj.is_missing) {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "[!] %s (Missing)", proj.name.c_str());
            } else {
                ImGui::TextColored(is_selected ? ImVec4(0.3f, 0.75f, 1.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", proj.name.c_str());
            }

            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 140.0f);
            ImGui::TextDisabled("%s", proj.last_modified.c_str());

            ImGui::TextDisabled("Folder: %s", proj.path.c_str());
            ImGui::EndGroup();

            if (ImGui::IsItemClicked()) {
                selected_project_idx = i;
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) && !proj.is_missing) {
                selected_project_idx = i;
                EditorState::get()->load_project_and_launch(proj.path);
            }

            ImGui::Separator();
            ImGui::PopID();
        }

        ImGui::EndChild();

        // Right Sidebar Actions
        ImGui::SameLine();
        ImGui::BeginChild("SidebarRegion", ImVec2(sidebar_width, content_height), true);

        bool has_selection = (selected_project_idx >= 0 && selected_project_idx < static_cast<int>(projects.size()));
        bool selected_valid = has_selection && !projects[selected_project_idx].is_missing;

        if (!selected_valid) ImGui::BeginDisabled();
        if (ImGui::Button("Edit Project", ImVec2(160.0f, 32.0f))) {
            if (selected_valid) {
                EditorState::get()->load_project_and_launch(projects[selected_project_idx].path);
            }
        }
        if (!selected_valid) ImGui::EndDisabled();

        ImGui::Spacing();

        if (!selected_valid) ImGui::BeginDisabled();
        if (ImGui::Button("Run Game", ImVec2(160.0f, 28.0f))) {
            if (selected_valid) {
                EditorState::get()->run_project_standalone(projects[selected_project_idx].path);
            }
        }
        if (!selected_valid) ImGui::EndDisabled();

        ImGui::Separator();

        if (!has_selection) ImGui::BeginDisabled();
        if (ImGui::Button("Rename", ImVec2(160.0f, 24.0f))) {
            snprintf(rename_buf, sizeof(rename_buf), "%s", projects[selected_project_idx].name.c_str());
            show_rename_modal = true;
        }
        if (ImGui::Button("Duplicate", ImVec2(160.0f, 24.0f))) {
            duplicate_project(selected_project_idx, projects[selected_project_idx].name + "_Copy");
        }
        if (ImGui::Button("Manage Tags", ImVec2(160.0f, 24.0f))) {
            show_tags_modal = true;
        }
        if (ImGui::Button("Remove", ImVec2(160.0f, 24.0f))) {
            show_remove_modal = true;
        }
        if (!has_selection) ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::Separator();

        if (ImGui::Button("Remove Missing", ImVec2(160.0f, 24.0f))) {
            remove_missing_projects();
        }

        ImGui::EndChild();
    } else {
        // Asset Library / Templates View
        ImGui::TextColored(ImVec4(0.3f, 0.75f, 1.0f, 1.0f), "Starter Templates & Demo Showcases");
        ImGui::TextDisabled("Select a template to generate a complete pre-configured RetroNode project.");
        ImGui::Spacing();

        struct TemplateCard {
            const char* title;
            const char* desc;
            int type_idx;
        } templates[] = {
            { "Top-Down RPG Starter", "Complete overworld scene with player movement, collision layers, dialogue UI, and audio stream players.", 2 },
            { "2D Platformer Starter", "Fixed 60Hz physics character body with jump physics, tilemap level, bounds culling, and camera tracking.", 1 },
            { "CPU Particle Showcase", "Particle emission shapes, scale curves, color ramp interpolation, and explosiveness dynamics.", 3 },
            { "Retro UI & Menu Demo", "NinePatchRect containers, labels, custom font texture rendering, and interactive controls.", 0 }
        };

        for (int i = 0; i < 4; ++i) {
            ImGui::PushID(i);
            ImGui::BeginChild(ImGui::GetID((void*)(intptr_t)i), ImVec2(ImGui::GetContentRegionAvail().x, 80.0f), true);
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", templates[i].title);
            ImGui::TextWrapped("%s", templates[i].desc);
            ImGui::SameLine(ImGui::GetWindowWidth() - 180.0f);
            if (ImGui::Button("Create Project", ImVec2(150.0f, 28.0f))) {
                snprintf(create_name_buf, sizeof(create_name_buf), "%s", templates[i].title);
                create_template_idx = templates[i].type_idx;
                show_create_modal = true;
            }
            ImGui::EndChild();
            ImGui::PopID();
            ImGui::Spacing();
        }
    }

    // Bottom Status Bar
    ImGui::Separator();
    ImGui::TextDisabled("RetroNode Engine v0.1.0-stable");
    ImGui::SameLine(ImGui::GetWindowWidth() - 300.0f);
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Status: %s", status_message.c_str());

    // --- MODALS ---

    // 1. Create Modal
    if (show_create_modal) {
        ImGui::OpenPopup("Create New RetroNode Project");
    }
    if (ImGui::BeginPopupModal("Create New RetroNode Project", &show_create_modal, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Project Name", create_name_buf, sizeof(create_name_buf));
        ImGui::InputText("Parent Folder", create_dir_buf, sizeof(create_dir_buf));

        const char* template_names[] = { "Empty Scene", "2D Platformer", "Top-Down RPG", "Particle Showcase" };
        ImGui::Combo("Template", &create_template_idx, template_names, 4);

        ImGui::Spacing();
        ImGui::Separator();

        if (ImGui::Button("Create & Open", ImVec2(140.0f, 28.0f))) {
            if (create_new_project(create_name_buf, create_dir_buf, create_template_idx)) {
                show_create_modal = false;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100.0f, 28.0f))) {
            show_create_modal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // 2. Import Modal
    if (show_import_modal) {
        ImGui::OpenPopup("Import Existing Project");
    }
    if (ImGui::BeginPopupModal("Import Existing Project", &show_import_modal, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Project Directory Path", import_path_buf, sizeof(import_path_buf));
        ImGui::Spacing();
        if (ImGui::Button("Import & Add", ImVec2(120.0f, 28.0f))) {
            if (add_project(import_path_buf)) {
                show_import_modal = false;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100.0f, 28.0f))) {
            show_import_modal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // 3. Scan Modal
    if (show_scan_modal) {
        ImGui::OpenPopup("Scan Directory for Projects");
    }
    if (ImGui::BeginPopupModal("Scan Directory for Projects", &show_scan_modal, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Folder Path", scan_path_buf, sizeof(scan_path_buf));
        ImGui::Spacing();
        if (ImGui::Button("Start Scan", ImVec2(120.0f, 28.0f))) {
            scan_directory(scan_path_buf[0] != '\0' ? scan_path_buf : ".");
            show_scan_modal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100.0f, 28.0f))) {
            show_scan_modal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // 4. Rename Modal
    if (show_rename_modal) {
        ImGui::OpenPopup("Rename Project");
    }
    if (ImGui::BeginPopupModal("Rename Project", &show_rename_modal, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("New Project Name", rename_buf, sizeof(rename_buf));
        ImGui::Spacing();
        if (ImGui::Button("Save Name", ImVec2(120.0f, 28.0f))) {
            rename_project(selected_project_idx, rename_buf);
            show_rename_modal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100.0f, 28.0f))) {
            show_rename_modal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // 5. Remove Modal
    if (show_remove_modal) {
        ImGui::OpenPopup("Remove Project");
    }
    if (ImGui::BeginPopupModal("Remove Project", &show_remove_modal, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Remove project '%s' from the library?", projects[selected_project_idx].name.c_str());
        ImGui::Checkbox("Also delete project files permanently from disk", &remove_delete_files);
        ImGui::Spacing();
        if (ImGui::Button("Confirm Remove", ImVec2(140.0f, 28.0f))) {
            remove_project(selected_project_idx, remove_delete_files);
            show_remove_modal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100.0f, 28.0f))) {
            show_remove_modal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // 6. Settings Modal
    if (show_settings_modal) {
        ImGui::OpenPopup("Engine Settings");
    }
    if (ImGui::BeginPopupModal("Engine Settings", &show_settings_modal, ImGuiWindowFlags_AlwaysAutoResize)) {
        float ui_sc = EditorState::get()->get_ui_scale();
        if (ImGui::SliderFloat("UI Scale", &ui_sc, 0.75f, 2.0f, "%.2f")) {
            EditorState::get()->set_ui_scale(ui_sc);
        }
        ImGui::Spacing();
        if (ImGui::Button("Close", ImVec2(100.0f, 28.0f))) {
            show_settings_modal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

} // namespace RetroNode
