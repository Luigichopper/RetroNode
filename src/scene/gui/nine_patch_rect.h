#ifndef RETRONODE_NINE_PATCH_RECT_H
#define RETRONODE_NINE_PATCH_RECT_H

#include "control.h"
#include <string>
#include <SDL3/SDL.h>

namespace RetroNode {

class RN_API NinePatchRect : public Control {
    RN_CLASS(NinePatchRect, Control)

public:
    std::string texture_path;
    uint32_t texture_id = 0;
    int patch_margin = 4;
    SDL_Color modulate = { 255, 255, 255, 255 };

    NinePatchRect();
    virtual ~NinePatchRect() = default;

    void set_texture_path(const std::string& path);
    virtual void _process(float delta) override;
};

} // namespace RetroNode

#endif // RETRONODE_NINE_PATCH_RECT_H
