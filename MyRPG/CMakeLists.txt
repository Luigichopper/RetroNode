cmake_minimum_required(VERSION 3.20)
project(MyRPG VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GAME_SOURCES
    src/module_init.cpp
    src/player_controller.cpp
)

add_library(game SHARED ${GAME_SOURCES})

target_include_directories(game PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(game PRIVATE retronode_core)

set_target_properties(game PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    PREFIX ""
)

find_package(Python3 COMPONENTS Interpreter QUIET)
if (Python3_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../tools/texture_packer.py")
    add_custom_target(compile_game_assets ALL
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../tools/texture_packer.py ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../tools/map_compiler.py ${CMAKE_CURRENT_SOURCE_DIR}/scenes/player.json ${CMAKE_CURRENT_SOURCE_DIR}/scenes/player.rnb
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../tools/map_compiler.py ${CMAKE_CURRENT_SOURCE_DIR}/scenes/overworld.json ${CMAKE_CURRENT_SOURCE_DIR}/scenes/overworld.rnb
        COMMENT "Auto-compiling sprite atlases and binary maps for ${PROJECT_NAME}..."
    )
    add_dependencies(game compile_game_assets)
endif()

