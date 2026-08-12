#ifndef RETRONODE_FILE_DIALOG_H
#define RETRONODE_FILE_DIALOG_H

#include <string>
#include <vector>
#include <functional>
#include <filesystem>

namespace RetroNode {

enum class FileDialogMode {
    OPEN_FILE,
    SELECT_FOLDER
};

class FileDialog {
private:
    static FileDialog* instance;

    bool is_open = false;
    std::string dialog_title = "Select File";
    std::string current_dir;
    std::string selected_file_path;
    std::string filter_pattern = "*.*";
    FileDialogMode mode = FileDialogMode::OPEN_FILE;
    std::function<void(const std::string&)> on_select_callback;

    char search_buf[128] = "";
    int view_mode = 0; // 0 = List View, 1 = Thumbnail Grid View
    float thumbnail_size = 64.0f;
    std::vector<std::string> dir_history;
    size_t history_index = 0;

    void navigate_to(const std::string& path);

public:
    FileDialog();
    ~FileDialog() = default;

    static FileDialog* get() {
        if (!instance) {
            instance = new FileDialog();
        }
        return instance;
    }

    void open(
        const std::string& title,
        const std::string& filter = "*.*",
        const std::string& initial_path = "",
        std::function<void(const std::string&)> callback = nullptr
    );

    void draw();

    // Helper: Normalize absolute path to "res://" relative path
    static std::string normalize_path(const std::string& full_path);

    // Helper Widget: Input box with [📁 Browse] button
    static bool draw_path_picker(
        const char* label,
        std::string& path_value,
        const char* filter = "*.*",
        float width = -1.0f
    );

    static bool draw_path_picker_buf(
        const char* label,
        char* buffer,
        size_t buffer_size,
        const char* filter = "*.*",
        float width = -1.0f
    );
};

} // namespace RetroNode

#endif // RETRONODE_FILE_DIALOG_H
