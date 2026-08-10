#include "timer.h"

namespace RetroNode {

RN_REGISTER_CLASS(Timer);

Timer::Timer() {
    name = "Timer";
}

void Timer::_ready() {
    if (autostart) {
        start();
    }
}

void Timer::start(float p_time) {
    if (p_time > 0.0f) {
        wait_time = p_time;
    }
    time_left = wait_time;
    is_stopped_flag = false;
}

void Timer::stop() {
    is_stopped_flag = true;
    time_left = 0.0f;
}

void Timer::_process(float delta) {
    Node::_process(delta);

    if (is_stopped_flag) return;

    time_left -= delta;
    if (time_left <= 0.0f) {
        if (on_timeout) {
            on_timeout();
        }

        if (one_shot) {
            stop();
        } else {
            time_left += wait_time;
        }
    }
}

} // namespace RetroNode
