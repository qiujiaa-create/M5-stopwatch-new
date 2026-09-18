#pragma once

#include "focus_timer.h"
#include <atomic>
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
    std::function<void(int)> onSaveCountdownMinutes;
    std::function<void(bool)> onRingChanged;
    bool isEditingDuration() const { return _editorOpen.load(); }

private:
    static void primaryClicked(lv_event_t* event);
    static void resetClicked(lv_event_t* event);
    static void countUpClicked(lv_event_t* event);
    static void countdownClicked(lv_event_t* event);
    static void minutesClicked(lv_event_t* event);
    static void ringClicked(lv_event_t* event);
    static void sliderChanged(lv_event_t* event);
    static void minusClicked(lv_event_t* event);
    static void plusClicked(lv_event_t* event);
    static void cancelClicked(lv_event_t* event);
    static void saveClicked(lv_event_t* event);
    void showEditor(int minutes);
    void setEditorMinutes(int minutes);

    lv_obj_t* _root = nullptr;
    lv_obj_t* _accentDisc = nullptr;
    lv_obj_t* _editorAccentDisc = nullptr;
    lv_obj_t* _status = nullptr;
    lv_obj_t* _hero = nullptr;
    lv_obj_t* _seconds = nullptr;
    lv_obj_t* _tenths = nullptr;
    lv_obj_t* _secondsUnderline = nullptr;
    lv_obj_t* _minuteHitbox = nullptr;
    lv_obj_t* _minuteHint = nullptr;
    lv_obj_t* _countUpChip = nullptr;
    lv_obj_t* _countdownChip = nullptr;
    lv_obj_t* _countUpChipLabel = nullptr;
    lv_obj_t* _countdownChipLabel = nullptr;
    lv_obj_t* _primaryLabel = nullptr;
    lv_obj_t* _primaryButton = nullptr;
    lv_obj_t* _resetButton = nullptr;
    lv_obj_t* _ringButton = nullptr;
    lv_obj_t* _ringIcon = nullptr;
    lv_obj_t* _ringLabel = nullptr;
    lv_obj_t* _footer = nullptr;
    lv_obj_t* _editor = nullptr;
    lv_obj_t* _editorValue = nullptr;
    lv_obj_t* _editorSlider = nullptr;
    std::string _shownHero;
    std::string _shownSeconds;
    std::string _shownTenths;
    std::string _shownStatus;
    std::string _shownAction;
    std::string _shownCountdownChip;
    std::string _shownFooter;
    bool _shownRingEnabled = false;
    std::atomic<bool> _editorOpen{false};
    int _displayedCountdownMinutes = FocusTimer::DefaultCountdownMinutes;
    Mode _shownMode = Mode::Countdown;
    bool _shownSplitHours = false;
    bool _shownTenthsVisible = false;
    bool _shownResetAvailable = false;
    bool _firstRender = true;
};

}  // namespace focus
