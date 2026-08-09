cmake_minimum_required(VERSION 3.20)
project(RetroNode VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Fetch dependencies
include(FetchContent)

set(SDL_TEST_LIBRARY OFF CACHE BOOL "Disable SDL3 tests" FORCE)
set(SDL_SHARED ON CACHE BOOL "Build SDL3 as shared library" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.8
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(SDL3 nlohmann_json stb)

# Engine Core Shared Library
set(ENGINE_CORE_SOURCES
    src/platform/game_module.cpp
    src/core/object/object.cpp
    src/core/object/class_db.cpp
    src/servers/input.cpp
    src/servers/visual_server.cpp
    src/servers/texture_server.cpp
    src/servers/physics_server.cpp
    src/servers/audio_server.cpp
    src/scene/main/node.cpp
    src/scene/main/scene_tree.cpp
    src/scene/main/scene_loader.cpp
    src/scene/2d/node_2d.cpp
    src/scene/2d/sprite_2d.cpp
    src/scene/2d/tile_map_layer.cpp
    src/scene/2d/camera_2d.cpp
    src/scene/physics/character_body_2d.cpp
    src/scene/gui/control.cpp
    src/scene/gui/canvas_layer.cpp
    src/scene/gui/label.cpp
    src/scene/gui/nine_patch_rect.cpp
    src/scene/gui/debug_overlay.cpp
    src/scene/audio/audio_stream_player.cpp
)

add_library(retronode_core SHARED ${ENGINE_CORE_SOURCES})

target_compile_definitions(retronode_core PRIVATE RN_BUILD_ENGINE)

target_include_directories(retronode_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${stb_SOURCE_DIR}
)

target_link_libraries(retronode_core PUBLIC nlohmann_json::nlohmann_json)

if (TARGET SDL3::SDL3-shared)
    target_link_libraries(retronode_core PUBLIC SDL3::SDL3-shared)
elseif (TARGET SDL3::SDL3)
    target_link_libraries(retronode_core PUBLIC SDL3::SDL3)
else()
    target_link_libraries(retronode_core PUBLIC SDL3)
endif()

if (WIN32)
    target_compile_definitions(retronode_core PUBLIC RN_PLATFORM_WINDOWS)
else()
    target_link_libraries(retronode_core PUBLIC ${CMAKE_DL_LIBS})
endif()

set_target_properties(retronode_core PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Debug"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Debug"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Debug"
)

# Automated Asset Compilation Target (runs texture_packer & map_compiler)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
if (Python3_FOUND)
    add_custom_target(compile_game_assets ALL
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/texture_packer.py ${CMAKE_CURRENT_SOURCE_DIR}/MyRPG
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/map_compiler.py ${CMAKE_CURRENT_SOURCE_DIR}/MyRPG/scenes/player.json ${CMAKE_CURRENT_SOURCE_DIR}/MyRPG/scenes/player.rnb
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/map_compiler.py ${CMAKE_CURRENT_SOURCE_DIR}/MyRPG/scenes/overworld.json ${CMAKE_CURRENT_SOURCE_DIR}/MyRPG/scenes/overworld.rnb
        COMMENT "Auto-compiling sprite atlases and binary maps..."
    )
endif()

# Engine Main Executable
add_executable(retronode src/platform/main.cpp)
target_link_libraries(retronode PRIVATE retronode_core)

if (TARGET compile_game_assets)
    add_dependencies(retronode compile_game_assets)
endif()

set_target_properties(retronode PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Debug"
)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/MyRPG/CMakeLists.txt")
    add_subdirectory(MyRPG)
endif()
