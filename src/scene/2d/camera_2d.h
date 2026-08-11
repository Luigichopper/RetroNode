#ifndef RETRONODE_CAMERA_2D_H
#define RETRONODE_CAMERA_2D_H

#include "node_2d.h"
#include "../../core/string_names.h"

namespace RetroNode {

enum class CameraMode {
    FOLLOW = 0,
    ZELDA_ROOM = 1,
    SMB1_FORWARD = 2,
    ANCHORED = 3
};

class RN_API Camera2D : public Node2D {
    RN_CLASS(Camera2D, Node2D)

private:
    static Camera2D* current_camera;

public:
    bool current = true;
    int mode = static_cast<int>(CameraMode::FOLLOW);

    // Smooth Following
    bool smoothing_enabled = true;
    float smoothing_speed = 10.0f;

    // Drag Margins (Deadzone Box inside Viewport)
    bool drag_margin_enabled = true;
    float drag_margin_left = 0.2f;   // Percentage of virtual width (0.0 to 0.5)
    float drag_margin_top = 0.2f;    // Percentage of virtual height (0.0 to 0.5)
    float drag_margin_right = 0.2f;
    float drag_margin_bottom = 0.2f;

    // Zelda Room Grid
    int room_width = 256;
    int room_height = 224;

    // Camera Limits
    bool limit_enabled = true;
    float limit_left = 0.0f;
    float limit_top = 0.0f;
    float limit_right = 1024.0f;
    float limit_bottom = 1024.0f;

    // SMB1 Forward Scroll State
    float max_forward_x = 0.0f;

    // Screen Shake
    float shake_strength = 0.0f;
    float shake_timer = 0.0f;

    // Property Getters & Setters for Reflection / Inspector
    void set_current(bool p_current);
    bool is_current() const { return current; }

    void set_mode(int p_mode) { mode = p_mode; }
    int get_mode() const { return mode; }

    void set_smoothing_enabled(bool b) { smoothing_enabled = b; }
    bool is_smoothing_enabled() const { return smoothing_enabled; }

    void set_smoothing_speed(float f) { smoothing_speed = f; }
    float get_smoothing_speed() const { return smoothing_speed; }

    void set_drag_margin_enabled(bool b) { drag_margin_enabled = b; }
    bool is_drag_margin_enabled() const { return drag_margin_enabled; }

    void set_drag_margin_left(float f) { drag_margin_left = f; }
    float get_drag_margin_left() const { return drag_margin_left; }

    void set_drag_margin_top(float f) { drag_margin_top = f; }
    float get_drag_margin_top() const { return drag_margin_top; }

    void set_drag_margin_right(float f) { drag_margin_right = f; }
    float get_drag_margin_right() const { return drag_margin_right; }

    void set_drag_margin_bottom(float f) { drag_margin_bottom = f; }
    float get_drag_margin_bottom() const { return drag_margin_bottom; }

    void set_room_width(int w) { room_width = w; }
    int get_room_width() const { return room_width; }

    void set_room_height(int h) { room_height = h; }
    int get_room_height() const { return room_height; }

    void set_limit_enabled(bool b) { limit_enabled = b; }
    bool is_limit_enabled() const { return limit_enabled; }

    void set_limit_left(float f) { limit_left = f; }
    float get_limit_left() const { return limit_left; }

    void set_limit_top(float f) { limit_top = f; }
    float get_limit_top() const { return limit_top; }

    void set_limit_right(float f) { limit_right = f; }
    float get_limit_right() const { return limit_right; }

    void set_limit_bottom(float f) { limit_bottom = f; }
    float get_limit_bottom() const { return limit_bottom; }

    void shake(float p_strength, float p_duration);
    void make_current();

    static Camera2D* get_current() { return current_camera; }

    Camera2D();
    virtual ~Camera2D();

    virtual void get_property_list(std::vector<PropertyInfo>& out_list) const override;
    virtual Variant get(const StringName& p_name) const override;
    virtual bool set(const StringName& p_name, const Variant& p_value) override;

    virtual void _process(float delta) override;
};


} // namespace RetroNode

#endif // RETRONODE_CAMERA_2D_H
