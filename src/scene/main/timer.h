#ifndef RETRONODE_TIMER_H
#define RETRONODE_TIMER_H

#include "node.h"
#include <functional>

namespace RetroNode {

class RN_API Timer : public Node {
    RN_CLASS(Timer, Node)

private:
    float wait_time = 1.0f;
    float time_left = 0.0f;
    bool one_shot = false;
    bool autostart = false;
    bool is_stopped_flag = true;

public:
    std::function<void()> on_timeout;

    Timer();
    virtual ~Timer() = default;

    void set_wait_time(float p_time) { wait_time = (p_time > 0.0f) ? p_time : 0.001f; }
    float get_wait_time() const { return wait_time; }

    void set_one_shot(bool p_one_shot) { one_shot = p_one_shot; }
    bool is_one_shot() const { return one_shot; }

    void set_autostart(bool p_autostart) { autostart = p_autostart; }
    bool has_autostart() const { return autostart; }

    void start(float p_time = -1.0f);
    void stop();
    bool is_stopped() const { return is_stopped_flag; }
    float get_time_left() const { return is_stopped_flag ? 0.0f : time_left; }

    virtual void _ready() override;
    virtual void _process(float delta) override;
};

} // namespace RetroNode

#endif // RETRONODE_TIMER_H
