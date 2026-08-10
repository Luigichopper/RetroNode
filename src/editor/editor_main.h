#ifndef RETRONODE_EDITOR_MAIN_H
#define RETRONODE_EDITOR_MAIN_H

#include <SDL3/SDL.h>
#include "../core/object/object.h"

namespace RetroNode {

class RN_API EditorMain {
private:
    static EditorMain* instance;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool initialized = false;

public:
    EditorMain() = default;
    ~EditorMain();

    static EditorMain* get() {
        if (!instance) {
            instance = new EditorMain();
        }
        return instance;
    }

    bool init(SDL_Window* p_window, SDL_Renderer* p_renderer);
    bool process_event(const SDL_Event& event);
    void render_frame(float alpha);
    void shutdown();

    bool is_initialized() const { return initialized; }
};

} // namespace RetroNode

#endif // RETRONODE_EDITOR_MAIN_H
