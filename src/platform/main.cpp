#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "core/math/fixed16.h"
#include "core/object/class_db.h"
#include "servers/visual_server.h"
#include "servers/physics_server.h"
#include "servers/texture_server.h"
#include "servers/input.h"
#include "scene/main/scene_tree.h"
#include "scene/main/scene_loader.h"
#include "scene/2d/node_2d.h"
#include "scene/2d/sprite_2d.h"
#include "scene/2d/tile_map_layer.h"
#include "scene/physics/character_body_2d.h"
#include "scene/2d/camera_2d.h"
#include "scene/gui/control.h"
#include "scene/gui/canvas_layer.h"
#include "scene/gui/label.h"
#include "scene/gui/nine_patch_rect.h"
#include "scene/gui/debug_overlay.h"
#include "scene/audio/audio_stream_player.h"
#include "servers/audio_server.h"
#include "game_module.h"

#include <iostream>
#include <filesystem>
#include <vector>

using namespace RetroNode;
namespace fs = std::filesystem;

void register_engine_classes() {
    RN_REGISTER_CLASS(Node);
    RN_REGISTER_CLASS(Node2D);
    RN_REGISTER_CLASS(Sprite2D);
    RN_REGISTER_CLASS(AnimatedSprite2D);
    RN_REGISTER_CLASS(TileMapLayer);
    RN_REGISTER_CLASS(CharacterBody2D);
    RN_REGISTER_CLASS(Camera2D);
    RN_REGISTER_CLASS(Control);
    RN_REGISTER_CLASS(CanvasLayer);
    RN_REGISTER_CLASS(Label);
    RN_REGISTER_CLASS(NinePatchRect);
    RN_REGISTER_CLASS(DebugOverlay);
    RN_REGISTER_CLASS(AudioStreamPlayer);
}

std::string resolve_project_dir(const std::string& input_dir, int argc, char* argv[]) {
    if (!input_dir.empty() && fs::exists(input_dir) && fs::is_directory(input_dir)) {
        return input_dir;
    }

    std::vector<std::string> candidates = {
        input_dir,
        "./MyRPG",
        "../MyRPG",
        "../../MyRPG",
        "../../../MyRPG"
    };

    if (argc > 0 && argv[0]) {
        std::error_code ec;
        fs::path exe_dir = fs::path(argv[0]).parent_path();
        candidates.push_back((exe_dir / "MyRPG").string());
        candidates.push_back((exe_dir / ".." / "MyRPG").string());
        candidates.push_back((exe_dir / ".." / ".." / "MyRPG").string());
        candidates.push_back((exe_dir / ".." / ".." / ".." / "MyRPG").string());
    }

    for (const auto& cand : candidates) {
        if (!cand.empty() && fs::exists(cand) && fs::is_directory(cand)) {
            if (fs::exists(cand + "/project.rnode") || fs::exists(cand + "/scenes") || fs::exists(cand + "/src")) {
                return cand;
            }
        }
    }

    return input_dir.empty() ? "./MyRPG" : input_dir;
}

std::string find_game_dll(const std::string& proj_dir) {
    std::vector<std::string> candidates = {
        proj_dir + "/bin/Debug/game.dll",
        proj_dir + "/bin/game.dll",
        proj_dir + "/bin/Release/game.dll",
        "./MyRPG/bin/Debug/game.dll",
        "../MyRPG/bin/Debug/game.dll",
        "../../MyRPG/bin/Debug/game.dll",
        "./game.dll"
    };

    for (const auto& path : candidates) {
        if (fs::exists(path)) {
            return path;
        }
    }

    return proj_dir + "/bin/Debug/game.dll";
}

std::string find_scene_file(const std::string& proj_dir) {
    std::vector<std::string> candidates = {
        proj_dir + "/scenes/overworld.rnb",
        proj_dir + "/scenes/overworld.json",
        "./MyRPG/scenes/overworld.rnb",
        "./MyRPG/scenes/overworld.json",
        "../MyRPG/scenes/overworld.rnb",
        "../MyRPG/scenes/overworld.json"
    };

    for (const auto& path : candidates) {
        if (fs::exists(path)) {
            return path;
        }
    }

    return proj_dir + "/scenes/overworld.json";
}

int main(int argc, char* argv[]) {
    std::cout << "==========================================" << std::endl;
    std::cout << "        RetroNode 2D Engine v0.1.0        " << std::endl;
    std::cout << "==========================================" << std::endl;

    // Eagerly initialize engine singletons
    ClassDB::get();
    Input::get();
    PhysicsServer2D::get();
    VisualServer::get();
    TextureServer::get();
    AudioServer::get();
    SceneTree::get();

    register_engine_classes();

    // Determine game project directory
    std::string specified_dir = "";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--project" && i + 1 < argc) {
            specified_dir = argv[++i];
        }
    }

    std::string project_dir = resolve_project_dir(specified_dir, argc, argv);
    std::string game_dll_path = find_game_dll(project_dir);

    std::cout << "[RetroNode Engine] Project directory: " << project_dir << std::endl;
    std::cout << "[RetroNode Engine] Game module path:  " << game_dll_path << std::endl;

    TextureServer::get()->set_project_dir(project_dir);
    AudioServer::get()->set_project_dir(project_dir);
    AudioServer::get()->init();

    // Load dynamic game logic module
    GameModuleLoader module_loader(game_dll_path);
    module_loader.load_module();

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "[RetroNode Engine] Failed to initialize SDL3: " << SDL_GetError() << std::endl;
        return -1;
    }

    const int WINDOW_WIDTH = 1024;
    const int WINDOW_HEIGHT = 896;

    SDL_Window* window = SDL_CreateWindow(
        "RetroNode Engine - MyRPG",
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        std::cerr << "[RetroNode Engine] Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        std::cerr << "[RetroNode Engine] Failed to create renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Enable VSync to cap framerate and eliminate 100% CPU busy-wait spin
    SDL_SetRenderVSync(renderer, 1);

    VisualServer::get()->init(renderer, 256, 224);

    // Auto-load texture atlas manifest if present
    TextureServer::get()->load_atlas_manifest("res://assets/atlas.json");

    // Load initial scene from JSON
    std::string scene_path = find_scene_file(project_dir);
    Node* root_scene = SceneLoader::load_scene_from_file(scene_path);
    if (root_scene) {
        SceneTree::get()->set_root(root_scene);
        std::cout << "[RetroNode Engine] Loaded initial scene: " << scene_path << std::endl;
    } else {
        std::cerr << "[RetroNode Engine] Warning: Could not load scene " << scene_path << std::endl;
    }

    const Fixed16 FIXED_DT = Fixed16::from_float(1.0f / 60.0f);
    const float FIXED_DT_FLOAT = FIXED_DT.to_float();
    const Fixed16 MAX_FRAME_TIME = Fixed16::from_float(0.25f);
    Fixed16 accumulator = Fixed16(0);

    uint64_t last_counter = SDL_GetPerformanceCounter();
    uint64_t frequency = SDL_GetPerformanceFrequency();

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            Input::get()->handle_event(event);
        }

        // Check for DLL hot-reloading
        module_loader.check_and_hot_reload();

        uint64_t current_counter = SDL_GetPerformanceCounter();
        float delta_seconds = static_cast<float>(current_counter - last_counter) / static_cast<float>(frequency);
        last_counter = current_counter;

        Fixed16 frame_time = Fixed16::from_float(delta_seconds);
        if (frame_time > MAX_FRAME_TIME) {
            frame_time = MAX_FRAME_TIME;
        }

        accumulator += frame_time;

        // Clear render queue once per frame before physics & frame process
        VisualServer::get()->clear_render_queue();

        // Fixed 60Hz Physics Step
        while (accumulator >= FIXED_DT) {
            SceneTree::get()->physics_process(FIXED_DT);
            accumulator -= FIXED_DT;
        }

        float render_alpha = accumulator.to_float() / FIXED_DT_FLOAT;
        
        // Frame Process & Render Step
        SceneTree::get()->process(delta_seconds);
        VisualServer::get()->render(render_alpha);
    }

    VisualServer::get()->shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
