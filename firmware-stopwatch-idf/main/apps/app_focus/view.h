#pragma once

#include "focus_timer.h"
#include <cstdint>
#include <functional>
#include <lvgl.h>
#include <string>

namespace focus {

class FocusView {
public:
    ~FocusView();
    void init(lv_obj_t* parent);
    void render(const FocusTimer& timer, uint64_t nowMs);

    std::function<void()> onPrimary;
    std::function<void()> onReset;
    std::function<void()> onSelectCountUp;
    std::function<void()> onSelectCountdown;

private:
    static void primaryClicked(lv_event_t* event);
    static void resetClicked(lv_event_t* event);
    static void countUpClicked(lv_event_t* event);
    static void countdownClicked(lv_event_t* event);

    lv_obj_t* _root = nullptr;
    lv_obj_t* _status = nullptr;
    lv_obj_t* _hero = nullptr;
    lv_obj_t* _seconds = nullptr;
    lv_obj_t* _tenths = nullptr;
    lv_obj_t* _secondsUnderline = nullptr;
    lv_obj_t* _countUpChip = nullptr;
    lv_obj_t* _countdownChip = nullptr;
    lv_obj_t* _countUpChipLabel = nullptr;
    lv_obj_t* _countdownChipLabel = nullptr;
    lv_obj_t* _primaryLabel = nullptr;
    lv_obj_t* _primaryButton = nullptr;
    lv_obj_t* _resetButton = nullptr;
    std::string _shownHero;
    std::string _shownSeconds;
    std::string _shownTenths;
    std::string _shownStatus;
    std::string _shownAction;
    Mode _shownMode = Mode::Countdown;
    bool _shownSplitHours = false;
    bool _shownTenthsVisible = false;
    bool _shownResetAvailable = false;
    bool _firstRender = true;
};

}  // namespace focus
