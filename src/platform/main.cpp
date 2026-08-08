#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "core/math/fixed16.h"
#include "core/object/class_db.h"
#include "servers/visual_server.h"
#include "servers/physics_server.h"
#include "servers/input.h"
#include "scene/main/scene_tree.h"
#include "scene/main/scene_loader.h"
#include "scene/2d/node_2d.h"
#include "scene/2d/sprite_2d.h"
#include "scene/physics/character_body_2d.h"
#include "scene/2d/camera_2d.h"
#include "game_module.h"

#include <iostream>
#include <filesystem>

using namespace RetroNode;
namespace fs = std::filesystem;

void register_engine_classes() {
    RN_REGISTER_CLASS(Node);
    RN_REGISTER_CLASS(Node2D);
    RN_REGISTER_CLASS(Sprite2D);
    RN_REGISTER_CLASS(AnimatedSprite2D);
    RN_REGISTER_CLASS(CharacterBody2D);
    RN_REGISTER_CLASS(Camera2D);
}

int main(int argc, char* argv[]) {
    std::cout << "==========================================" << std::endl;
    std::cout << "        RetroNode 2D Engine v0.1.0        " << std::endl;
    std::cout << "==========================================" << std::endl;

    register_engine_classes();

    // Determine game project directory
    std::string project_dir = "./MyRPG";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--project" && i + 1 < argc) {
            project_dir = argv[++i];
        }
    }

    std::string game_dll_path = project_dir + "/bin/Debug/game.dll";
    if (!fs::exists(game_dll_path)) {
        game_dll_path = project_dir + "/bin/game.dll";
    }

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

    VisualServer::get()->init(renderer, 256, 224);

    // Load initial scene from JSON
    std::string scene_path = project_dir + "/scenes/overworld.json";
    Node* root_scene = SceneLoader::load_scene_from_file(scene_path);
    if (root_scene) {
        SceneTree::get()->set_root(root_scene);
        std::cout << "[RetroNode Engine] Loaded initial scene: " << scene_path << std::endl;
    } else {
        std::cerr << "[RetroNode Engine] Warning: Could not load scene " << scene_path << std::endl;
    }

    const Fixed16 FIXED_DT = Fixed16::from_float(1.0f / 60.0f);
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

        // Fixed 60Hz Physics Step
        while (accumulator >= FIXED_DT) {
            SceneTree::get()->physics_process(FIXED_DT);
            accumulator -= FIXED_DT;
        }

        float render_alpha = accumulator.to_float() / FIXED_DT.to_float();
        
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
