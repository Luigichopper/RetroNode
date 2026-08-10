#include "debug_overlay.h"
#include "../../servers/visual_server.h"
#include "../../servers/texture_server.h"
#include "../../servers/input.h"
#include <sstream>

namespace RetroNode {

DebugOverlay::DebugOverlay() {
    name = "DebugOverlay";
    position = Vector2Fixed::from_floats(4.0f, 4.0f);
    size = Vector2Fixed::from_floats(248.0f, 32.0f);
    z_index = 200; // Always render on top of all UI
}

void DebugOverlay::_ready() {
    fps_label = nullptr;
    info_label = nullptr;

    for (Node* child : get_children()) {
        if (child->get_name() == "FPSLabel") {
            fps_label = dynamic_cast<Label*>(child);
        } else if (child->get_name() == "InfoLabel") {
            info_label = dynamic_cast<Label*>(child);
        }
    }

    if (!fps_label) {
        fps_label = new Label();
        fps_label->set_name("FPSLabel");
        fps_label->set_position(Vector2Fixed::from_floats(0.0f, 0.0f));
        fps_label->text_color = { 255, 230, 80, 255 }; // Retro gold yellow
        fps_label->set_text("FPS: 60 (16.6ms)");
        add_child(fps_label);
    }

    if (!info_label) {
        info_label = new Label();
        info_label->set_name("InfoLabel");
        info_label->set_position(Vector2Fixed::from_floats(0.0f, 10.0f));
        info_label->text_color = { 100, 220, 255, 255 }; // Light cyan
        info_label->set_text("DRAWS: 0  TEX: 0");
        add_child(info_label);
    }
}

void DebugOverlay::_process(float delta) {
    Control::_process(delta);

    // Toggle debug overlay visibility with F3 key
    if (Input::get()->is_action_pressed(StringName("toggle_debug"))) {
        visible = !visible;
    }

    if (!visible) return;

    frame_count++;
    time_accumulator += delta;

    if (time_accumulator >= 0.25f) {
        current_fps = static_cast<float>(frame_count) / time_accumulator;
        float frame_ms = (current_fps > 0.0f) ? (1000.0f / current_fps) : 0.0f;
        frame_count = 0;
        time_accumulator = 0.0f;

        if (fps_label) {
            std::stringstream ss;
            ss << "FPS: " << static_cast<int>(current_fps + 0.5f) << " (" << static_cast<int>(frame_ms + 0.5f) << "ms)";
            fps_label->set_text(ss.str());
        }

        if (info_label) {
            size_t draws = VisualServer::get()->get_draw_call_count();
            size_t textures = TextureServer::get()->get_texture_count();
            std::stringstream ss;
            ss << "DRAWS:" << draws << " TEX:" << textures;
            info_label->set_text(ss.str());
        }
    }
}

} // namespace RetroNode
