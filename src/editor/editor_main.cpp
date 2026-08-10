#include "editor_main.h"
#include "editor_state.h"
#include "panels/main_menu_bar.h"
#include "panels/scene_tree_panel.h"
#include "panels/inspector_panel.h"
#include "panels/filesystem_panel.h"
#include "panels/viewport_panel.h"
#include "../servers/visual_server.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <iostream>

namespace RetroNode {

EditorMain* EditorMain::instance = nullptr;

EditorMain::~EditorMain() {
    shutdown();
}

bool EditorMain::init(SDL_Window* p_window, SDL_Renderer* p_renderer) {
    window = p_window;
    renderer = p_renderer;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;

    // Dark Retro Slate Color Theme Setup
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.22f, 0.27f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.32f, 0.40f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.40f, 0.50f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.22f, 0.27f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.32f, 0.40f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.40f, 0.50f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.09f, 0.11f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.14f, 0.14f, 0.18f, 1.0f);

    if (!ImGui_ImplSDL3_InitForSDLRenderer(window, renderer)) {
        std::cerr << "[Editor] Failed to initialize ImGui SDL3 backend" << std::endl;
        return false;
    }

    if (!ImGui_ImplSDLRenderer3_Init(renderer)) {
        std::cerr << "[Editor] Failed to initialize ImGui SDL_Renderer3 backend" << std::endl;
        return false;
    }

    initialized = true;
    std::cout << "[Editor] Initialized Dear ImGui Visual Editor Shell successfully." << std::endl;
    return true;
}

bool EditorMain::process_event(const SDL_Event& event) {
    if (!initialized) return false;
    ImGui_ImplSDL3_ProcessEvent(&event);

    ImGuiIO& io = ImGui::GetIO();
    bool is_play_mode = EditorState::get()->get_is_play_mode();

    // Event routing condition: gating gameplay input
    if (is_play_mode && !io.WantCaptureKeyboard && !io.WantCaptureMouse) {
        return false; // Forward event to gameplay
    }
    return true;
}

void EditorMain::render_frame(float alpha) {
    if (!initialized || !renderer) return;

    // 1. Render engine draw commands into virtual_framebuffer
    VisualServer::get()->render_scene(alpha);

    // 2. Start ImGui Frame
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // 3. Establish DockSpace over main viewport
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    // 4. Render Docks & Menu Bar
    MainMenuBar::draw();
    SceneTreePanel::draw();
    InspectorPanel::draw();
    FileSystemPanel::draw();
    ViewportPanel::draw();

    // 5. Render ImGui Draw Commands to Window Screen
    ImGui::Render();

    SDL_SetRenderTarget(renderer, NULL);
    SDL_SetRenderDrawColor(renderer, 20, 20, 24, 255);
    SDL_RenderClear(renderer);

    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
}

void EditorMain::shutdown() {
    if (!initialized) return;
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    initialized = false;
    std::cout << "[Editor] ImGui Visual Editor shut down cleanly." << std::endl;
}

} // namespace RetroNode
