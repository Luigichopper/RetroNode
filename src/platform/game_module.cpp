#include "game_module.h"
#include "../core/object/class_db.h"
#include "../scene/main/scene_tree.h"
#include "../scene/main/scene_loader.h"
#include "../servers/physics_server.h"

#include <iostream>
#include <fstream>
#include <filesystem>

#if !defined(_WIN32)
  #include <dlfcn.h>
#endif

namespace fs = std::filesystem;

namespace RetroNode {

GameModuleLoader::GameModuleLoader(const std::string& p_module_path) 
    : module_path(p_module_path) {
    temp_module_path = module_path + ".temp.dll";
}

GameModuleLoader::~GameModuleLoader() {
    unload_module();
}

uint64_t GameModuleLoader::get_file_write_time(const std::string& path) {
    std::error_code ec;
    if (fs::exists(path, ec)) {
        auto ftime = fs::last_write_time(path, ec);
        return static_cast<uint64_t>(ftime.time_since_epoch().count());
    }
    return 0;
}

bool GameModuleLoader::load_module() {
    unload_module();

    if (!fs::exists(module_path)) {
        std::cerr << "[GameModuleLoader] Shared library path does not exist: " << module_path << std::endl;
        return false;
    }

    last_modified_time = get_file_write_time(module_path);

#if defined(_WIN32)
    std::error_code ec;
    fs::copy_file(module_path, temp_module_path, fs::copy_options::overwrite_existing, ec);
    std::string path_to_load = ec ? module_path : temp_module_path;

    handle = LoadLibraryA(path_to_load.c_str());
    if (!handle) {
        std::cerr << "[GameModuleLoader] Failed to load DLL: " << GetLastError() << std::endl;
        return false;
    }

    RegisterTypesFunc register_fn = (RegisterTypesFunc)GetProcAddress(handle, "retronode_register_types");
    if (register_fn) {
        std::cout << "[GameModuleLoader] Invoking retronode_register_types(ClassDB*)..." << std::endl;
        register_fn(ClassDB::get());
    } else {
        std::cerr << "[GameModuleLoader] Warning: 'retronode_register_types' symbol not found in DLL" << std::endl;
    }
#else
    handle = dlopen(module_path.c_str(), RTLD_NOW);
    if (!handle) {
        std::cerr << "[GameModuleLoader] Failed to dlopen shared object: " << dlerror() << std::endl;
        return false;
    }

    RegisterTypesFunc register_fn = (RegisterTypesFunc)dlsym(handle, "retronode_register_types");
    if (register_fn) {
        register_fn(ClassDB::get());
    }
#endif

    std::cout << "[GameModuleLoader] Successfully loaded game module: " << module_path << std::endl;
    return true;
}

void GameModuleLoader::unload_module() {
    if (handle) {
#if defined(_WIN32)
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
        handle = nullptr;
    }
}

bool GameModuleLoader::check_and_hot_reload() {
    uint64_t current_time = get_file_write_time(module_path);
    if (current_time > last_modified_time && current_time != 0) {
        std::cout << "[GameModuleLoader] Detected modified module! Safely resetting scene tree..." << std::endl;
        PhysicsServer2D::get()->clear();
        SceneTree::get()->set_root(nullptr);
        bool success = load_module();
        if (success && !scene_path.empty()) {
            Node* new_root = SceneLoader::load_scene_from_file(scene_path);
            if (new_root) {
                SceneTree::get()->set_root(new_root);
                std::cout << "[GameModuleLoader] Hot-reloaded scene: " << scene_path << std::endl;
            } else {
                std::cerr << "[GameModuleLoader] Failed to reload scene after module update: " << scene_path << std::endl;
            }
        }
        return success;
    }
    return false;
}

} // namespace RetroNode
