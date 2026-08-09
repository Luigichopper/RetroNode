#ifndef RETRONODE_DEBUG_OVERLAY_H
#define RETRONODE_DEBUG_OVERLAY_H

#include "control.h"
#include "label.h"

namespace RetroNode {

class RN_API DebugOverlay : public Control {
    RN_CLASS(DebugOverlay, Control)

private:
    Label* fps_label = nullptr;
    Label* info_label = nullptr;
    
    int frame_count = 0;
    float time_accumulator = 0.0f;
    float current_fps = 60.0f;

public:
    DebugOverlay();
    virtual ~DebugOverlay() = default;

    virtual void _ready() override;
    virtual void _process(float delta) override;
};

} // namespace RetroNode

#endif // RETRONODE_DEBUG_OVERLAY_H
