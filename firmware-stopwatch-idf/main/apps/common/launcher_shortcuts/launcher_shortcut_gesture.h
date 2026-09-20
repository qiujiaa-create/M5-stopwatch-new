#pragma once

#include <cstdint>

namespace launcher_shortcuts {

enum class GestureEvent {
    None,
    ArmA,
    ArmB,
    LaunchA,
    LaunchB,
    Cancelled,
};

class ShortcutGesture {
public:
    explicit ShortcutGesture(uint32_t holdMs) : _hold_ms(holdMs) {}

    GestureEvent update(uint32_t nowMs, bool buttonAPressed, bool buttonBPressed)
    {
        if (!buttonAPressed && !buttonBPressed) {
            const auto launch_event = _armed_event == GestureEvent::ArmA
                                          ? GestureEvent::LaunchA
                                          : (_armed_event == GestureEvent::ArmB ? GestureEvent::LaunchB
                                                                                : GestureEvent::None);
            reset();
            return launch_event;
        }

        if (buttonAPressed && buttonBPressed) {
            const bool cancel_armed_action = _armed_event != GestureEvent::None;
            _chord_active = true;
            _a_tracking = false;
            _b_tracking = false;
            _armed_event = GestureEvent::None;
            return cancel_armed_action ? GestureEvent::Cancelled : GestureEvent::None;
        }

        if (_chord_active || _armed_event != GestureEvent::None) {
            return GestureEvent::None;
        }

        if (buttonAPressed) {
            _b_tracking = false;
            if (!_a_tracking) {
                _a_tracking = true;
                _a_started_ms = nowMs;
            } else if (nowMs - _a_started_ms >= _hold_ms) {
                _armed_event = GestureEvent::ArmA;
                return _armed_event;
            }
        } else {
            _a_tracking = false;
        }

        if (buttonBPressed) {
            _a_tracking = false;
            if (!_b_tracking) {
                _b_tracking = true;
                _b_started_ms = nowMs;
            } else if (nowMs - _b_started_ms >= _hold_ms) {
                _armed_event = GestureEvent::ArmB;
                return _armed_event;
            }
        } else {
            _b_tracking = false;
        }

        return GestureEvent::None;
    }

    void reset()
    {
        _a_tracking = false;
        _b_tracking = false;
        _chord_active = false;
        _armed_event = GestureEvent::None;
        _a_started_ms = 0;
        _b_started_ms = 0;
    }

private:
    uint32_t _hold_ms = 0;
    uint32_t _a_started_ms = 0;
    uint32_t _b_started_ms = 0;
    bool _a_tracking = false;
    bool _b_tracking = false;
    bool _chord_active = false;
    GestureEvent _armed_event = GestureEvent::None;
};

}  // namespace launcher_shortcuts
