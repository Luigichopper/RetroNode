#ifndef RETRONODE_PROJECT_MANAGER_H
#define RETRONODE_PROJECT_MANAGER_H

#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <unordered_set>
#include "../core/object/class_db.h"

namespace RetroNode {

struct ProjectInfo {
    std::string name;
    std::string path;
    std::string main_scene = "scenes/main.json";
    std::string icon_path;
    bool is_favorite = false;
    std::vector<std::string> tags;
    std::string last_modified = "Just now";
    std::string engine_version = "v0.1.0";
    bool is_missing = false;
};

enum class ProjectManagerTab {
    PROJECTS = 0,
    ASSET_LIBRARY = 1
};

class RN_API ProjectManager {
private:
    static ProjectManager* instance;

    std::vector<ProjectInfo> projects;
    int selected_project_idx = -1;

    ProjectManagerTab current_tab = ProjectManagerTab::PROJECTS;

    // Filter & Sort State
    char filter_buf[128] = "";
    int sort_mode = 0; // 0 = Last Edited, 1 = Name, 2 = Path, 3 = Favorites First

    // Modal dialog visibility states
    bool show_create_modal = false;
    bool show_import_modal = false;
    bool show_scan_modal = false;
    bool show_rename_modal = false;
    bool show_tags_modal = false;
    bool show_remove_modal = false;
    bool show_settings_modal = false;

    // Modal input buffers
    char create_name_buf[128] = "New Project";
    char create_dir_buf[256] = "";
    int create_template_idx = 0;

    char import_path_buf[256] = "";
    char scan_path_buf[256] = "";
    char rename_buf[128] = "";
    char new_tag_buf[64] = "";

    bool remove_delete_files = false;

    std::string status_message = "Ready";

    std::string get_config_filepath() const;
    void sort_projects();

public:
    ProjectManager();
    ~ProjectManager();

    static ProjectManager* get() {
        if (!instance) {
            instance = new ProjectManager();
        }
        return instance;
    }

    void init();
    void load_projects_json();
    void save_projects_json();

    void scan_directory(const std::string& root_dir);
    bool add_project(const std::string& project_dir_path);
    void remove_project(int index, bool delete_files_from_disk);
    void remove_missing_projects();

    bool create_new_project(const std::string& proj_name, const std::string& parent_dir, int template_idx);
    bool duplicate_project(int index, const std::string& new_name);
    bool rename_project(int index, const std::string& new_name);

    void draw_ui(SDL_Window* window, SDL_Renderer* renderer);

    const std::vector<ProjectInfo>& get_projects() const { return projects; }
    int get_selected_index() const { return selected_project_idx; }

    void set_status(const std::string& msg) { status_message = msg; }
};

} // namespace RetroNode

#endif // RETRONODE_PROJECT_MANAGER_H
