#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "core/math/fixed16.h"
#include <iostream>

using RetroNode::Fixed16;

int main(int argc, char* argv[]) {
    std::cout << "[RetroNode Engine] Bootstrapping..." << std::endl;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "[RetroNode Engine] Failed to initialize SDL3: " << SDL_GetError() << std::endl;
        return -1;
    }

    const int WINDOW_WIDTH = 1024;
    const int WINDOW_HEIGHT = 896;

    SDL_Window* window = SDL_CreateWindow(
        "RetroNode Engine",
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        std::cerr << "[RetroNode Engine] Failed to create SDL3 window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        std::cerr << "[RetroNode Engine] Failed to create SDL3 renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    std::cout << "[RetroNode Engine] Window created (1024x896). Fixed 60Hz loop running." << std::endl;

    // Fixed 60Hz Physics Step (1/60th second)
    const Fixed16 FIXED_DT = Fixed16::from_float(1.0f / 60.0f);
    const Fixed16 MAX_FRAME_TIME = Fixed16::from_float(0.25f);
    Fixed16 accumulator = Fixed16(0);

    uint64_t last_counter = SDL_GetPerformanceCounter();
    uint64_t frequency = SDL_GetPerformanceFrequency();

    bool running = true;
    SDL_Event event;

    uint64_t tick_count = 0;

    while (running) {
        // Handle input and OS events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Calculate delta time
        uint64_t current_counter = SDL_GetPerformanceCounter();
        float delta_seconds = static_cast<float>(current_counter - last_counter) / static_cast<float>(frequency);
        last_counter = current_counter;

        Fixed16 frame_time = Fixed16::from_float(delta_seconds);
        if (frame_time > MAX_FRAME_TIME) {
            frame_time = MAX_FRAME_TIME;
        }

        accumulator += frame_time;

        // Fixed 60Hz Physics Accumulator Loop
        while (accumulator >= FIXED_DT) {
            // Physics / fixed update step
            tick_count++;
            accumulator -= FIXED_DT;
        }

        // Interpolation alpha for rendering
        float render_alpha = accumulator.to_float() / FIXED_DT.to_float();
        (void)render_alpha; // To be passed to VisualServer in full implementation

        // Render pass (retro dark theme clear)
        SDL_SetRenderDrawColor(renderer, 24, 20, 36, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    std::cout << "[RetroNode Engine] Shutdown complete. Total physics ticks: " << tick_count << std::endl;

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
