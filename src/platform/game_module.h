#ifndef RETRONODE_GAME_MODULE_H
#define RETRONODE_GAME_MODULE_H

#include <string>
#include "../core/object/class_db.h"

#if defined(_WIN32)
  #include <windows.h>
  typedef HMODULE ModuleHandle;
#else
  typedef void* ModuleHandle;
#endif

namespace RetroNode {

class ClassDB;
typedef void (*RegisterTypesFunc)(ClassDB* db);

class RN_API GameModuleLoader {
private:
    std::string module_path;
    std::string temp_module_path;
    std::string scene_path;
    ModuleHandle handle = nullptr;
    uint64_t last_modified_time = 0;

    uint64_t get_file_write_time(const std::string& path);

public:
    GameModuleLoader(const std::string& p_module_path);
    ~GameModuleLoader();

    void set_scene_path(const std::string& p_scene_path) { scene_path = p_scene_path; }
    bool load_module();
    void unload_module();
    bool check_and_hot_reload();
};

} // namespace RetroNode

#endif // RETRONODE_GAME_MODULE_H
