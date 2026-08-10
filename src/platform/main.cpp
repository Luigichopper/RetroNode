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
#include "scene/animation/animation_player.h"
#include "scene/main/timer.h"
#include "scene/2d/marker_2d.h"
#include "scene/2d/cpu_particles_2d.h"
#include "servers/audio_server.h"
#include "game_module.h"

#ifdef RN_BUILD_EDITOR
#include "editor/editor_main.h"
#include "editor/editor_state.h"
#endif

#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>

using namespace RetroNode;
namespace fs = std::filesystem;

void register_engine_classes() {
    RN_REGISTER_CLASS(Node);
    ClassDB::register_property(
        "Node", PropertyInfo{ "name", VariantType::STRING },
        &Node::set_name, &Node::get_name
    );
    ClassDB::register_property(
        "Node", PropertyInfo{ "visible", VariantType::BOOL },
        &Node::set_visible, &Node::is_visible
    );

    RN_REGISTER_CLASS(Timer);
    ClassDB::register_property(
        "Timer", PropertyInfo{ "wait_time", VariantType::FLOAT16, PropertyHint::RANGE, "0.001,4096.0,0.01" },
        &Timer::set_wait_time, &Timer::get_wait_time
    );
    ClassDB::register_property(
        "Timer", PropertyInfo{ "one_shot", VariantType::BOOL },
        &Timer::set_one_shot, &Timer::is_one_shot
    );
    ClassDB::register_property(
        "Timer", PropertyInfo{ "autostart", VariantType::BOOL },
        &Timer::set_autostart, &Timer::has_autostart
    );

    RN_REGISTER_CLASS(Node2D);
    ClassDB::register_property(
        "Node2D", PropertyInfo{ "position", VariantType::VECTOR2 },
        &Node2D::set_position, &Node2D::get_position
    );
    ClassDB::register_property(
        "Node2D", PropertyInfo{ "rotation", VariantType::FLOAT16 },
        &Node2D::set_rotation, &Node2D::get_rotation
    );
    ClassDB::register_property(
        "Node2D", PropertyInfo{ "scale", VariantType::VECTOR2 },
        &Node2D::set_scale, &Node2D::get_scale
    );

    RN_REGISTER_CLASS(Marker2D);

    RN_REGISTER_CLASS(Sprite2D);
    ClassDB::register_property(
        "Sprite2D", PropertyInfo{ "texture_path", VariantType::STRING, PropertyHint::FILE_PATH, "*.png" },
        &Sprite2D::set_texture_path, &Sprite2D::get_texture_path
    );
    ClassDB::register_property(
        "Sprite2D", PropertyInfo{ "z_index", VariantType::INT },
        &Sprite2D::set_z_index, &Sprite2D::get_z_index
    );
    ClassDB::register_property(
        "Sprite2D", PropertyInfo{ "modulate", VariantType::COLOR },
        &Sprite2D::set_modulate, &Sprite2D::get_modulate
    );

    RN_REGISTER_CLASS(AnimatedSprite2D);
    ClassDB::register_property(
        "AnimatedSprite2D", PropertyInfo{ "current_animation", VariantType::STRING },
        &AnimatedSprite2D::set_current_animation, &AnimatedSprite2D::get_current_animation
    );

    RN_REGISTER_CLASS(TileMapLayer);
    ClassDB::register_property(
        "TileMapLayer", PropertyInfo{ "tileset_path", VariantType::STRING, PropertyHint::FILE_PATH, "*.png" },
        &TileMapLayer::set_tileset_path, &TileMapLayer::get_tileset_path
    );
    ClassDB::register_property(
        "TileMapLayer", PropertyInfo{ "z_index", VariantType::INT },
        &TileMapLayer::set_z_index, &TileMapLayer::get_z_index
    );
    ClassDB::register_property(
        "TileMapLayer", PropertyInfo{ "modulate", VariantType::COLOR },
        &TileMapLayer::set_modulate, &TileMapLayer::get_modulate
    );
    ClassDB::register_property(
        "TileMapLayer", PropertyInfo{ "columns", VariantType::INT },
        &TileMapLayer::set_columns, &TileMapLayer::get_columns
    );
    ClassDB::register_property(
        "TileMapLayer", PropertyInfo{ "rows", VariantType::INT },
        &TileMapLayer::set_rows, &TileMapLayer::get_rows
    );
    ClassDB::register_property(
        "TileMapLayer", PropertyInfo{ "tile_size", VariantType::INT },
        &TileMapLayer::set_tile_size, &TileMapLayer::get_tile_size
    );

    RN_REGISTER_CLASS(CharacterBody2D);
    ClassDB::register_property(
        "CharacterBody2D", PropertyInfo{ "velocity", VariantType::VECTOR2 },
        &CharacterBody2D::set_velocity, &CharacterBody2D::get_velocity
    );
    ClassDB::register_property(
        "CharacterBody2D", PropertyInfo{ "body_size", VariantType::VECTOR2 },
        &CharacterBody2D::set_body_size, &CharacterBody2D::get_body_size
    );

    RN_REGISTER_CLASS(Camera2D);
    ClassDB::register_property(
        "Camera2D", PropertyInfo{ "limit_enabled", VariantType::BOOL },
        &Camera2D::set_limit_enabled, &Camera2D::is_limit_enabled
    );
    ClassDB::register_property(
        "Camera2D", PropertyInfo{ "limit_left", VariantType::FLOAT16 },
        &Camera2D::set_limit_left, &Camera2D::get_limit_left
    );
    ClassDB::register_property(
        "Camera2D", PropertyInfo{ "limit_top", VariantType::FLOAT16 },
        &Camera2D::set_limit_top, &Camera2D::get_limit_top
    );
    ClassDB::register_property(
        "Camera2D", PropertyInfo{ "limit_right", VariantType::FLOAT16 },
        &Camera2D::set_limit_right, &Camera2D::get_limit_right
    );
    ClassDB::register_property(
        "Camera2D", PropertyInfo{ "limit_bottom", VariantType::FLOAT16 },
        &Camera2D::set_limit_bottom, &Camera2D::get_limit_bottom
    );

    RN_REGISTER_CLASS(Control);
    ClassDB::register_property(
        "Control", PropertyInfo{ "position", VariantType::VECTOR2 },
        &Control::set_position, &Control::get_position
    );
    ClassDB::register_property(
        "Control", PropertyInfo{ "size", VariantType::VECTOR2 },
        &Control::set_size, &Control::get_size
    );

    RN_REGISTER_CLASS(CanvasLayer);

    RN_REGISTER_CLASS(Label);
    ClassDB::register_property(
        "Label", PropertyInfo{ "text", VariantType::STRING },
        &Label::set_text, &Label::get_text
    );

    RN_REGISTER_CLASS(NinePatchRect);
    RN_REGISTER_CLASS(DebugOverlay);

    RN_REGISTER_CLASS(AudioStreamPlayer);
    ClassDB::register_property(
        "AudioStreamPlayer", PropertyInfo{ "stream_path", VariantType::STRING, PropertyHint::FILE_PATH, "*.wav" },
        &AudioStreamPlayer::set_stream_path, &AudioStreamPlayer::get_stream_path
    );
    ClassDB::register_property(
        "AudioStreamPlayer", PropertyInfo{ "volume", VariantType::FLOAT16 },
        &AudioStreamPlayer::set_volume, &AudioStreamPlayer::get_volume
    );
    ClassDB::register_property(
        "AudioStreamPlayer", PropertyInfo{ "autoplay", VariantType::BOOL },
        &AudioStreamPlayer::set_autoplay, &AudioStreamPlayer::has_autoplay
    );

    RN_REGISTER_CLASS(AnimationPlayer);

    RN_REGISTER_CLASS(CPUParticles2D);
    ClassDB::register_property("CPUParticles2D", PropertyInfo{ "emitting", VariantType::BOOL }, &CPUParticles2D::set_emitting, &CPUParticles2D::get_emitting);
    ClassDB::register_property("CPUParticles2D", PropertyInfo{ "amount", VariantType::INT }, &CPUParticles2D::set_amount, &CPUParticles2D::get_amount);
    ClassDB::register_property("CPUParticles2D", PropertyInfo{ "lifetime", VariantType::FLOAT16 }, &CPUParticles2D::set_lifetime, &CPUParticles2D::get_lifetime);
    ClassDB::register_property("CPUParticles2D", PropertyInfo{ "one_shot", VariantType::BOOL }, &CPUParticles2D::set_one_shot, &CPUParticles2D::get_one_shot);
    ClassDB::register_property("CPUParticles2D", PropertyInfo{ "speed_scale", VariantType::FLOAT16 }, &CPUParticles2D::set_speed_scale, &CPUParticles2D::get_speed_scale);
    ClassDB::register_property("CPUParticles2D", PropertyInfo{ "explosiveness", VariantType::FLOAT16 }, &CPUParticles2D::set_explosiveness, &CPUParticles2D::get_explosiveness);
    ClassDB::register_property("CPUParticles2D", PropertyInfo{ "direction", VariantType::VECTOR2 }, &CPUParticles2D::set_direction, &CPUParticles2D::get_direction);
    ClassDB::register_property("CPUParticles2D", PropertyInfo{ "spread", VariantType::FLOAT16 }, &CPUParticles2D::set_spread, &CPUParticles2D::get_spread);
    ClassDB::register_property("CPUParticles2D", PropertyInfo{ "gravity", VariantType::VECTOR2 }, &CPUParticles2D::set_gravity, &CPUParticles2D::get_gravity);
    ClassDB::register_property("CPUParticles2D", PropertyInfo{ "initial_velocity_min", VariantType::FLOAT16 }, &CPUParticles2D::set_initial_velocity_min, &CPUParticles2D::get_initial_velocity_min);
    ClassDB::register_property("CPUParticles2D", PropertyInfo{ "initial_velocity_max", VariantType::FLOAT16 }, &CPUParticles2D::set_initial_velocity_max, &CPUParticles2D::get_initial_velocity_max);
    ClassDB::register_property("CPUParticles2D", PropertyInfo{ "angular_velocity_min", VariantType::FLOAT16 }, &CPUParticles2D::set_angular_velocity_min, &CPUParticles2D::get_angular_velocity_min);
    ClassDB::register_property("CPUParticles2D", PropertyInfo{ "angular_velocity_max", VariantType::FLOAT16 }, &CPUParticles2D::set_angular_velocity_max, &CPUParticles2D::get_angular_velocity_max);
    ClassDB::register_property("CPUParticles2D", PropertyInfo{ "scale_amount_min", VariantType::FLOAT16 }, &CPUParticles2D::set_scale_amount_min, &CPUParticles2D::get_scale_amount_min);
    ClassDB::register_property("CPUParticles2D", PropertyInfo{ "scale_amount_max", VariantType::FLOAT16 }, &CPUParticles2D::set_scale_amount_max, &CPUParticles2D::get_scale_amount_max);
    ClassDB::register_property("CPUParticles2D", PropertyInfo{ "color", VariantType::COLOR }, &CPUParticles2D::set_color, &CPUParticles2D::get_color);
    ClassDB::register_property("CPUParticles2D", PropertyInfo{ "texture", VariantType::STRING, PropertyHint::FILE_PATH, "*.png" }, &CPUParticles2D::set_texture_path, &CPUParticles2D::get_texture_path);
    ClassDB::register_property("CPUParticles2D", PropertyInfo{ "z_index", VariantType::INT }, &CPUParticles2D::set_z_index, &CPUParticles2D::get_z_index);
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
    std::vector<std::string> search_dirs = {
        proj_dir + "/bin/Debug",
        proj_dir + "/bin",
        proj_dir + "/bin/Release",
        "./MyRPG/bin/Debug",
        "../MyRPG/bin/Debug",
        "../../MyRPG/bin/Debug",
        "."
    };

    std::vector<std::string> file_names = {
#if defined(_WIN32)
        "game.dll", "libgame.dll", "game.so", "libgame.so"
#elif defined(__APPLE__)
        "libgame.dylib", "game.dylib", "libgame.so", "game.so", "game.dll"
#else
        "libgame.so", "game.so", "game.dll", "libgame.dylib"
#endif
    };

    for (const auto& dir : search_dirs) {
        for (const auto& fname : file_names) {
            std::string candidate = dir + "/" + fname;
            if (fs::exists(candidate)) {
                return candidate;
            }
        }
    }

#if defined(_WIN32)
    return proj_dir + "/bin/Debug/game.dll";
#elif defined(__APPLE__)
    return proj_dir + "/bin/libgame.dylib";
#else
    return proj_dir + "/bin/libgame.so";
#endif
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

    bool editor_mode = false;
    std::string specified_dir = "";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--editor" || arg == "-e") {
            editor_mode = true;
        } else if (arg == "--project" && i + 1 < argc) {
            specified_dir = argv[++i];
        }
    }

    std::string project_dir = resolve_project_dir(specified_dir, argc, argv);
    std::string game_dll_path = find_game_dll(project_dir);

    // Parse project.rnode configuration file
    std::string project_name = "MyRPG";
    std::string configured_main_scene = "";
    int window_width = 1024;
    int window_height = 896;
    int virtual_width = 256;
    int virtual_height = 224;
    int target_fps = 60;

    std::string rnode_path = project_dir + "/project.rnode";
    if (fs::exists(rnode_path)) {
        try {
            std::ifstream rnode_file(rnode_path);
            if (rnode_file.is_open()) {
                nlohmann::json config;
                rnode_file >> config;
                if (config.contains("name") && config["name"].is_string()) {
                    project_name = config["name"];
                }
                if (config.contains("main_scene") && config["main_scene"].is_string()) {
                    configured_main_scene = config["main_scene"];
                }
                if (config.contains("display") && config["display"].is_object()) {
                    const auto& disp = config["display"];
                    window_width = disp.value("window_width", window_width);
                    window_height = disp.value("window_height", window_height);
                    virtual_width = disp.value("virtual_width", virtual_width);
                    virtual_height = disp.value("virtual_height", virtual_height);
                    target_fps = disp.value("target_fps", target_fps);
                }
                std::cout << "[RetroNode Engine] Parsed project configuration from: " << rnode_path << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[RetroNode Engine] Warning parsing project.rnode: " << e.what() << std::endl;
        }
    }

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

    std::string window_title = "RetroNode Engine - " + project_name;
    if (editor_mode) {
        window_title += " [Editor Mode]";
    }

    SDL_Window* window = SDL_CreateWindow(
        window_title.c_str(),
        window_width,
        window_height,
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

    VisualServer::get()->init(renderer, virtual_width, virtual_height);

    // Auto-load texture atlas manifest if present
    TextureServer::get()->load_atlas_manifest("res://assets/atlas.json");

    // Resolve initial scene path
    std::string scene_path = "";
    if (!configured_main_scene.empty()) {
        std::string resolved_path = configured_main_scene;
        if (resolved_path.rfind("res://", 0) == 0) {
            resolved_path = resolved_path.substr(6);
        }
        std::string full_path = project_dir + "/" + resolved_path;
        if (full_path.length() > 5 && full_path.substr(full_path.length() - 5) == ".json") {
            std::string rnb_path = full_path.substr(0, full_path.length() - 5) + ".rnb";
            if (fs::exists(rnb_path)) {
                scene_path = rnb_path;
            }
        }
        if (scene_path.empty() && fs::exists(full_path)) {
            scene_path = full_path;
        }
    }
    if (scene_path.empty()) {
        scene_path = find_scene_file(project_dir);
    }

    module_loader.set_scene_path(scene_path);

    Node* root_scene = SceneLoader::load_scene_from_file(scene_path);
    if (root_scene) {
        SceneTree::get()->set_root(root_scene);
        std::cout << "[RetroNode Engine] Loaded initial scene: " << scene_path << std::endl;
    } else {
        std::cerr << "[RetroNode Engine] Warning: Could not load scene " << scene_path << std::endl;
    }

#ifdef RN_BUILD_EDITOR
    if (editor_mode) {
        EditorState::get()->set_project_dir(project_dir);
        EditorState::get()->set_current_scene_path(scene_path);
        EditorMain::get()->init(window, renderer);
    }
#endif

    float fps_dt = (target_fps > 0) ? (1.0f / static_cast<float>(target_fps)) : (1.0f / 60.0f);
    const Fixed16 FIXED_DT = Fixed16::from_float(fps_dt);
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
#ifdef RN_BUILD_EDITOR
            if (editor_mode) {
                bool handled = EditorMain::get()->process_event(event);
                if (!handled) {
                    Input::get()->handle_event(event);
                }
            } else {
                Input::get()->handle_event(event);
            }
#else
            Input::get()->handle_event(event);
#endif
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

        bool is_physics_active = true;
#ifdef RN_BUILD_EDITOR
        if (editor_mode) {
            is_physics_active = EditorState::get()->get_is_play_mode() && !EditorState::get()->get_is_paused();
        }
#endif

        if (is_physics_active) {
            // Fixed Physics Step
            while (accumulator >= FIXED_DT) {
#ifdef RN_BUILD_EDITOR
                if (editor_mode && EditorState::get()->get_play_mode_root()) {
                    EditorState::get()->get_play_mode_root()->propagate_physics_process(FIXED_DT);
                } else {
                    SceneTree::get()->physics_process(FIXED_DT);
                }
#else
                SceneTree::get()->physics_process(FIXED_DT);
#endif
                accumulator -= FIXED_DT;
            }
        } else {
            accumulator = Fixed16(0);
        }

        // Visual draw commands populated every frame for editor viewport rendering
#ifdef RN_BUILD_EDITOR
        if (editor_mode && EditorState::get()->get_is_play_mode() && EditorState::get()->is_game_view_active() && EditorState::get()->get_play_mode_root()) {
            EditorState::get()->get_play_mode_root()->propagate_process(delta_seconds);
        } else {
            SceneTree::get()->process(delta_seconds);
        }
#else
        SceneTree::get()->process(delta_seconds);
#endif

        float render_alpha = accumulator.to_float() / FIXED_DT_FLOAT;
        
#ifdef RN_BUILD_EDITOR
        if (editor_mode) {
            EditorMain::get()->render_frame(render_alpha);
        } else {
            VisualServer::get()->render(render_alpha);
        }
#else
        VisualServer::get()->render(render_alpha);
#endif
    }

#ifdef RN_BUILD_EDITOR
    if (editor_mode) {
        EditorMain::get()->shutdown();
    }
#endif

    VisualServer::get()->shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
