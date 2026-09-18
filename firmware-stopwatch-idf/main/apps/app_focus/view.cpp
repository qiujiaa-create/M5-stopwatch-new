#include "view.h"
#include <assets/assets.h>
#include <algorithm>
#include <cstdio>

namespace focus {
namespace {
constexpr uint32_t kPaper = 0xF5EAD6;
constexpr uint32_t kInk = 0x050505;
constexpr uint32_t kCoral = 0xFF3B30;
constexpr uint32_t kCountdownBlue = 0x46D1E0;
constexpr uint32_t kYellow = 0xFFC400;

lv_obj_t* makeBox(lv_obj_t* parent, int x, int y, int width, int height,
                  uint32_t fill, int radius)
{
    lv_obj_t* box = lv_obj_create(parent);
    lv_obj_set_pos(box, x, y);
    lv_obj_set_size(box, width, height);
    lv_obj_set_style_bg_color(box, lv_color_hex(fill), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(box, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(box, radius, LV_PART_MAIN);
    lv_obj_set_style_pad_all(box, 0, LV_PART_MAIN);
    lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    return box;
}

lv_obj_t* makeLabel(lv_obj_t* parent, const char* value, int x, int y,
                    int width, int height, const lv_font_t* font,
                    uint32_t color, lv_text_align_t align)
{
    lv_obj_t* label = lv_label_create(parent);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_size(label, width, height);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_text_align(label, align, LV_PART_MAIN);
    lv_label_set_text(label, value);
    return label;
}

void setTextIfChanged(lv_obj_t* label, std::string& previous, const std::string& next)
{
    if (previous == next) return;
    previous = next;
    lv_label_set_text(label, next.c_str());
}
}  // namespace

FocusView::~FocusView()
{
    if (_root) lv_obj_delete(_root);
}

void FocusView::init(lv_obj_t* parent)
{
    _root = makeBox(parent, 0, 0, 466, 466, kPaper, 0);
    // The clipped coral disc and warm paper follow the reference focus page.
    _accentDisc = makeBox(_root, 212, -35, 336, 336, kCoral, LV_RADIUS_CIRCLE);
    lv_obj_set_style_bg_opa(_accentDisc, LV_OPA_COVER, LV_PART_MAIN);
    makeLabel(_root, "FOCUS", 96, 48, 160, 32, &MontserratSemiBold26, kInk, LV_TEXT_ALIGN_LEFT);
    _status = makeLabel(_root, "READY", 96, 81, 190, 34, &MontserratSemiBold26, kInk, LV_TEXT_ALIGN_LEFT);
    makeBox(_root, 96, 120, 53, 4, kInk, 0);

    _hero = makeLabel(_root, "00:00", 22, 151, 410, 126, &CommissionerMedium108,
                      kInk, LV_TEXT_ALIGN_LEFT);
    _seconds = makeLabel(_root, "", 356, 215, 60, 39, &MontserratSemiBold26,
                         kInk, LV_TEXT_ALIGN_CENTER);
    _tenths = makeLabel(_root, ".0", 340, 214, 62, 50, &lv_font_montserrat_36,
                        kInk, LV_TEXT_ALIGN_LEFT);
    lv_obj_add_flag(_tenths, LV_OBJ_FLAG_HIDDEN);
    _secondsUnderline = makeBox(_root, 366, 255, 42, 4, kInk, 0);
    lv_obj_add_flag(_secondsUnderline, LV_OBJ_FLAG_HIDDEN);

    _minuteHitbox = makeBox(_root, 24, 163, 151, 116, kPaper, 0);
    lv_obj_set_style_bg_opa(_minuteHitbox, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_add_flag(_minuteHitbox, LV_OBJ_FLAG_CLICKABLE);
    _minuteHint = makeLabel(_minuteHitbox, "TAP MIN TO SET", 0, 100, 151, 16,
                            &lv_font_montserrat_14, kInk, LV_TEXT_ALIGN_CENTER);
    lv_obj_add_event_cb(_minuteHitbox, minutesClicked, LV_EVENT_CLICKED, this);
    lv_obj_add_flag(_minuteHitbox, LV_OBJ_FLAG_HIDDEN);

    _countUpChip = makeBox(_root, 55, 287, 173, 52, kInk, 26);
    _countdownChip = makeBox(_root, 238, 287, 173, 52, kPaper, 26);
    lv_obj_set_style_border_width(_countdownChip, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(_countdownChip, lv_color_hex(kYellow), LV_PART_MAIN);
    _countUpChipLabel = makeLabel(_countUpChip, "A  COUNT UP", 0, 12, 173, 28,
                                 &lv_font_montserrat_18, kPaper, LV_TEXT_ALIGN_CENTER);
    _countdownChipLabel = makeLabel(_countdownChip, "B  25 MIN", 0, 12, 173, 28,
                                    &lv_font_montserrat_18, kInk, LV_TEXT_ALIGN_CENTER);
    lv_obj_add_flag(_countUpChip, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(_countdownChip, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(_countUpChip, countUpClicked, LV_EVENT_CLICKED, this);
    lv_obj_add_event_cb(_countdownChip, countdownClicked, LV_EVENT_CLICKED, this);

    _primaryButton = makeBox(_root, 131, 354, 205, 56, kInk, 28);
    lv_obj_add_flag(_primaryButton, LV_OBJ_FLAG_CLICKABLE);
    _primaryLabel = makeLabel(_primaryButton, "A  START", 0, 12, 205, 33,
                             &MontserratSemiBold26, kPaper, LV_TEXT_ALIGN_CENTER);
    lv_obj_add_event_cb(_primaryButton, primaryClicked, LV_EVENT_CLICKED, this);

    _resetButton = makeBox(_root, 309, 354, 86, 56, kPaper, 28);
    lv_obj_set_style_border_color(_resetButton, lv_color_hex(kInk), LV_PART_MAIN);
    lv_obj_set_style_border_width(_resetButton, 2, LV_PART_MAIN);
    lv_obj_add_flag(_resetButton, LV_OBJ_FLAG_CLICKABLE);
    makeLabel(_resetButton, "RESET", 0, 16, 86, 24, &lv_font_montserrat_16,
              kInk, LV_TEXT_ALIGN_CENTER);
    lv_obj_add_event_cb(_resetButton, resetClicked, LV_EVENT_CLICKED, this);
    lv_obj_add_flag(_resetButton, LV_OBJ_FLAG_HIDDEN);

    _ringButton = makeBox(_root, 102, 353, 262, 58, kPaper, 29);
    lv_obj_set_style_border_width(_ringButton, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(_ringButton, lv_color_hex(kInk), LV_PART_MAIN);
    lv_obj_add_flag(_ringButton, LV_OBJ_FLAG_CLICKABLE);
    _ringIcon = makeLabel(_ringButton, LV_SYMBOL_MUTE, 21, 16, 33, 29,
                          &lv_font_montserrat_24, kInk, LV_TEXT_ALIGN_CENTER);
    _ringLabel = makeLabel(_ringButton, "RING OFF", 62, 16, 176, 32,
                           &MontserratSemiBold26, kInk, LV_TEXT_ALIGN_CENTER);
    lv_obj_add_event_cb(_ringButton, ringClicked, LV_EVENT_CLICKED, this);
    lv_obj_add_flag(_ringButton, LV_OBJ_FLAG_HIDDEN);

    _footer = makeLabel(_root, "HOLD A WHEN PAUSED", 118, 422, 230, 22,
                        &lv_font_montserrat_14, kInk, LV_TEXT_ALIGN_CENTER);

    _editor = makeBox(_root, 0, 0, 466, 466, kPaper, 0);
    lv_obj_add_flag(_editor, LV_OBJ_FLAG_CLICKABLE);
    _editorAccentDisc = makeBox(_editor, 250, -48, 290, 290, kCountdownBlue, LV_RADIUS_CIRCLE);
    lv_obj_set_style_bg_opa(_editorAccentDisc, LV_OPA_COVER, LV_PART_MAIN);
    makeLabel(_editor, "SET TIMER", 93, 64, 280, 39,
              &MontserratSemiBold26, kInk, LV_TEXT_ALIGN_CENTER);
    makeLabel(_editor, "1 - 60 MIN", 103, 105, 260, 28,
              &lv_font_montserrat_20, kInk, LV_TEXT_ALIGN_CENTER);
    _editorValue = makeLabel(_editor, "25", 142, 145, 182, 112,
                             &CommissionerMedium108, kInk, LV_TEXT_ALIGN_CENTER);
    auto* minus = makeBox(_editor, 73, 174, 61, 61, kInk, 31);
    makeLabel(minus, "-", 0, 10, 61, 43, &lv_font_montserrat_36,
              kPaper, LV_TEXT_ALIGN_CENTER);
    lv_obj_add_flag(minus, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(minus, minusClicked, LV_EVENT_CLICKED, this);
    auto* plus = makeBox(_editor, 332, 174, 61, 61, kInk, 31);
    makeLabel(plus, "+", 0, 10, 61, 43, &lv_font_montserrat_36,
              kPaper, LV_TEXT_ALIGN_CENTER);
    lv_obj_add_flag(plus, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(plus, plusClicked, LV_EVENT_CLICKED, this);

    _editorSlider = lv_slider_create(_editor);
    lv_obj_set_pos(_editorSlider, 91, 279);
    lv_obj_set_size(_editorSlider, 284, 22);
    lv_slider_set_range(_editorSlider, FocusTimer::MinCountdownMinutes,
                        FocusTimer::MaxCountdownMinutes);
    lv_obj_set_style_bg_color(_editorSlider, lv_color_hex(kInk), LV_PART_MAIN);
    lv_obj_set_style_bg_color(_editorSlider, lv_color_hex(kCoral), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(_editorSlider, lv_color_hex(kInk), LV_PART_KNOB);
    lv_obj_add_event_cb(_editorSlider, sliderChanged, LV_EVENT_VALUE_CHANGED, this);
    makeLabel(_editor, "1", 90, 311, 35, 25, &lv_font_montserrat_18,
              kInk, LV_TEXT_ALIGN_LEFT);
    makeLabel(_editor, "60", 340, 311, 35, 25, &lv_font_montserrat_18,
              kInk, LV_TEXT_ALIGN_RIGHT);

    auto* cancel = makeBox(_editor, 90, 350, 132, 57, kPaper, 28);
    lv_obj_set_style_border_width(cancel, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(cancel, lv_color_hex(kInk), LV_PART_MAIN);
    makeLabel(cancel, "CANCEL", 0, 15, 132, 29, &lv_font_montserrat_20,
              kInk, LV_TEXT_ALIGN_CENTER);
    lv_obj_add_flag(cancel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(cancel, cancelClicked, LV_EVENT_CLICKED, this);
    auto* save = makeBox(_editor, 244, 350, 132, 57, kInk, 28);
    makeLabel(save, "SAVE", 0, 15, 132, 29, &lv_font_montserrat_20,
              kPaper, LV_TEXT_ALIGN_CENTER);
    lv_obj_add_flag(save, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(save, saveClicked, LV_EVENT_CLICKED, this);
    makeLabel(_editor, "SAVE RESETS CURRENT TIMER", 107, 421, 252, 21,
              &lv_font_montserrat_14, kInk, LV_TEXT_ALIGN_CENTER);
    lv_obj_add_flag(_editor, LV_OBJ_FLAG_HIDDEN);
}

void FocusView::render(const FocusTimer& timer, uint64_t nowMs)
{
    const Mode mode = timer.selectedMode();
    const TimerState state = mode == Mode::CountUp ? timer.countUpState() : timer.countdownState();
    const uint64_t elapsedMs = timer.countUpElapsedMs(nowMs);
    const uint64_t totalSeconds = mode == Mode::CountUp
        ? elapsedMs / 1000 : (timer.countdownRemainingMs(nowMs) + 999) / 1000;
    const bool splitHours = mode == Mode::CountUp && totalSeconds >= 3600;
    const uint64_t hours = splitHours ? totalSeconds / 3600 : 0;
    const uint64_t minutes = splitHours ? (totalSeconds / 60) % 60 : totalSeconds / 60;
    const uint64_t seconds = totalSeconds % 60;
    char big[40];
    char small[4] = {};
    if (splitHours) {
        std::snprintf(big, sizeof(big), "%02llu:%02llu",
                      static_cast<unsigned long long>(hours),
                      static_cast<unsigned long long>(minutes));
        std::snprintf(small, sizeof(small), "%02llu", static_cast<unsigned long long>(seconds));
    } else {
        std::snprintf(big, sizeof(big), "%02llu:%02llu",
                      static_cast<unsigned long long>(minutes),
                      static_cast<unsigned long long>(seconds));
    }
    const bool heroChanged = _shownHero != big;
    if (_firstRender || _shownSplitHours != splitHours) {
        _shownSplitHours = splitHours;
        if (_shownSplitHours) {
            lv_obj_remove_flag(_secondsUnderline, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_text_font(_tenths, &lv_font_montserrat_20, LV_PART_MAIN);
            lv_obj_set_pos(_tenths, 414, 224);
        } else {
            lv_obj_add_flag(_secondsUnderline, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_text_font(_tenths, &lv_font_montserrat_36, LV_PART_MAIN);
        }
    }
    if (heroChanged && !_shownSplitHours) {
        lv_point_t heroSize;
        lv_text_get_size(&heroSize, big, &CommissionerMedium108, 0, 0,
                         LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        lv_obj_set_pos(_tenths, std::min<int32_t>(22 + heroSize.x + 6, 385), 213);
    }
    setTextIfChanged(_hero, _shownHero, big);
    setTextIfChanged(_seconds, _shownSeconds, small);
    if (_firstRender || _shownTenthsVisible != (mode == Mode::CountUp)) {
        _shownTenthsVisible = mode == Mode::CountUp;
        if (_shownTenthsVisible) lv_obj_remove_flag(_tenths, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(_tenths, LV_OBJ_FLAG_HIDDEN);
    }
    if (mode == Mode::CountUp) {
        char tenths[4];
        std::snprintf(tenths, sizeof(tenths), ".%u", static_cast<unsigned>((elapsedMs / 100) % 10));
        setTextIfChanged(_tenths, _shownTenths, tenths);
    }

    const char* status = state == TimerState::Running ? "RUNNING" :
                         state == TimerState::Paused ? "PAUSED" :
                         state == TimerState::Completed ? "DONE" : "READY";
    setTextIfChanged(_status, _shownStatus, status);
    const char* action = state == TimerState::Running ? "PAUSE" :
                         state == TimerState::Paused ? "RESUME" : "START";
    setTextIfChanged(_primaryLabel, _shownAction,
                     std::string(mode == Mode::CountUp ? "A  " : "B  ") + action);
    char chip[24];
    _displayedCountdownMinutes = timer.countdownMinutes();
    std::snprintf(chip, sizeof(chip), "B  %d MIN", timer.countdownMinutes());
    setTextIfChanged(_countdownChipLabel, _shownCountdownChip, chip);
    if (_firstRender || _shownResetAvailable != (mode == Mode::CountUp && state == TimerState::Paused)) {
        _shownResetAvailable = mode == Mode::CountUp && state == TimerState::Paused;
        if (_shownResetAvailable) {
            lv_obj_set_x(_primaryButton, 92);
            lv_obj_remove_flag(_resetButton, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_set_x(_primaryButton, 131);
            lv_obj_add_flag(_resetButton, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (_firstRender || _shownRingEnabled != timer.ringEnabled()) {
        _shownRingEnabled = timer.ringEnabled();
        lv_obj_set_style_bg_color(_ringButton,
                                  lv_color_hex(_shownRingEnabled ? kInk : kPaper), LV_PART_MAIN);
        const uint32_t foreground = _shownRingEnabled ? kPaper : kInk;
        lv_obj_set_style_text_color(_ringIcon, lv_color_hex(foreground), LV_PART_MAIN);
        lv_obj_set_style_text_color(_ringLabel, lv_color_hex(foreground), LV_PART_MAIN);
        lv_label_set_text(_ringIcon, _shownRingEnabled ? LV_SYMBOL_VOLUME_MAX : LV_SYMBOL_MUTE);
        lv_label_set_text(_ringLabel, _shownRingEnabled ? "RING ON" : "RING OFF");
    }

    if (_firstRender || mode != _shownMode) {
        _shownMode = mode;
        _firstRender = false;
        lv_obj_set_style_bg_color(_accentDisc,
                                  lv_color_hex(mode == Mode::Countdown ? kCountdownBlue : kCoral),
                                  LV_PART_MAIN);
        if (mode == Mode::Countdown) {
            lv_obj_add_flag(_primaryButton, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(_resetButton, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(_ringButton, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(_minuteHitbox, LV_OBJ_FLAG_HIDDEN);
            setTextIfChanged(_footer, _shownFooter, "B KEY / HOLD TO RESET");
        } else {
            lv_obj_remove_flag(_primaryButton, LV_OBJ_FLAG_HIDDEN);
            if (_shownResetAvailable) lv_obj_remove_flag(_resetButton, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(_ringButton, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(_minuteHitbox, LV_OBJ_FLAG_HIDDEN);
            setTextIfChanged(_footer, _shownFooter, "HOLD A WHEN PAUSED");
        }
        lv_obj_set_style_bg_color(_countUpChip, lv_color_hex(mode == Mode::CountUp ? kInk : kPaper), LV_PART_MAIN);
        lv_obj_set_style_border_width(_countUpChip, mode == Mode::CountUp ? 0 : 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(_countUpChip, lv_color_hex(kYellow), LV_PART_MAIN);
        lv_obj_set_style_text_color(_countUpChipLabel,
                                    lv_color_hex(mode == Mode::CountUp ? kPaper : kInk), LV_PART_MAIN);
        lv_obj_set_style_bg_color(_countdownChip, lv_color_hex(mode == Mode::Countdown ? kInk : kPaper), LV_PART_MAIN);
        lv_obj_set_style_border_width(_countdownChip, mode == Mode::Countdown ? 0 : 2, LV_PART_MAIN);
        lv_obj_set_style_text_color(_countdownChipLabel,
                                    lv_color_hex(mode == Mode::Countdown ? kPaper : kInk), LV_PART_MAIN);
    }
}

void FocusView::primaryClicked(lv_event_t* event)
{
    auto* view = static_cast<FocusView*>(lv_event_get_user_data(event));
    if (view->onPrimary) view->onPrimary();
}

void FocusView::resetClicked(lv_event_t* event)
{
    auto* view = static_cast<FocusView*>(lv_event_get_user_data(event));
    if (view->onReset) view->onReset();
}

void FocusView::countUpClicked(lv_event_t* event)
{
    auto* view = static_cast<FocusView*>(lv_event_get_user_data(event));
    if (view->onSelectCountUp) view->onSelectCountUp();
}

void FocusView::countdownClicked(lv_event_t* event)
{
    auto* view = static_cast<FocusView*>(lv_event_get_user_data(event));
    if (view->onSelectCountdown) view->onSelectCountdown();
}

void FocusView::showEditor(int minutes)
{
    setEditorMinutes(minutes);
    _editorOpen.store(true);
    lv_obj_remove_flag(_editor, LV_OBJ_FLAG_HIDDEN);
}

void FocusView::setEditorMinutes(int minutes)
{
    const int clamped = std::clamp(minutes, FocusTimer::MinCountdownMinutes,
                                   FocusTimer::MaxCountdownMinutes);
    lv_slider_set_value(_editorSlider, clamped, LV_ANIM_OFF);
    char value[4];
    std::snprintf(value, sizeof(value), "%d", clamped);
    lv_label_set_text(_editorValue, value);
}

void FocusView::minutesClicked(lv_event_t* event)
{
    auto* view = static_cast<FocusView*>(lv_event_get_user_data(event));
    view->showEditor(view->_displayedCountdownMinutes);
}

void FocusView::ringClicked(lv_event_t* event)
{
    auto* view = static_cast<FocusView*>(lv_event_get_user_data(event));
    if (view->onRingChanged) view->onRingChanged(!view->_shownRingEnabled);
}

void FocusView::sliderChanged(lv_event_t* event)
{
    auto* view = static_cast<FocusView*>(lv_event_get_user_data(event));
    char value[4];
    std::snprintf(value, sizeof(value), "%ld",
                  static_cast<long>(lv_slider_get_value(view->_editorSlider)));
    lv_label_set_text(view->_editorValue, value);
}

void FocusView::minusClicked(lv_event_t* event)
{
    auto* view = static_cast<FocusView*>(lv_event_get_user_data(event));
    view->setEditorMinutes(lv_slider_get_value(view->_editorSlider) - 1);
}

void FocusView::plusClicked(lv_event_t* event)
{
    auto* view = static_cast<FocusView*>(lv_event_get_user_data(event));
    view->setEditorMinutes(lv_slider_get_value(view->_editorSlider) + 1);
}

void FocusView::cancelClicked(lv_event_t* event)
{
    auto* view = static_cast<FocusView*>(lv_event_get_user_data(event));
    lv_obj_add_flag(view->_editor, LV_OBJ_FLAG_HIDDEN);
    view->_editorOpen.store(false);
}

void FocusView::saveClicked(lv_event_t* event)
{
    auto* view = static_cast<FocusView*>(lv_event_get_user_data(event));
    if (view->onSaveCountdownMinutes) {
        view->onSaveCountdownMinutes(lv_slider_get_value(view->_editorSlider));
    }
    lv_obj_add_flag(view->_editor, LV_OBJ_FLAG_HIDDEN);
    view->_editorOpen.store(false);
}
}  // namespace focus
