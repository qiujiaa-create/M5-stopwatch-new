#include "view.h"
#include <assets/assets.h>
#include <algorithm>
#include <cstdio>

namespace focus {
namespace {
constexpr uint32_t kPaper = 0xF5EAD6;
constexpr uint32_t kInk = 0x050505;
constexpr uint32_t kCoral = 0xFF3B30;
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
    makeBox(_root, 212, -35, 336, 336, kCoral, LV_RADIUS_CIRCLE);
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

    makeLabel(_root, "HOLD A/B WHEN PAUSED", 123, 422, 220, 22,
              &lv_font_montserrat_14, kInk, LV_TEXT_ALIGN_CENTER);
}

void FocusView::render(const FocusTimer& timer, uint64_t nowMs)
{
    const Mode mode = timer.selectedMode();
    const TimerState state = mode == Mode::CountUp ? timer.countUpState() : timer.countdownState();
    const uint64_t elapsedMs = timer.countUpElapsedMs(nowMs);
    const uint64_t totalSeconds = mode == Mode::CountUp
        ? elapsedMs / 1000 : (timer.countdownRemainingMs(nowMs) + 999) / 1000;
    const uint64_t hours = totalSeconds / 3600;
    const uint64_t minutes = (totalSeconds / 60) % 60;
    const uint64_t seconds = totalSeconds % 60;
    char big[20];
    char small[4] = {};
    if (hours > 0) {
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
    if (_firstRender || _shownSplitHours != (hours > 0)) {
        _shownSplitHours = hours > 0;
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
    if (_firstRender || _shownResetAvailable != (state == TimerState::Paused)) {
        _shownResetAvailable = state == TimerState::Paused;
        if (_shownResetAvailable) {
            lv_obj_set_x(_primaryButton, 92);
            lv_obj_remove_flag(_resetButton, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_set_x(_primaryButton, 131);
            lv_obj_add_flag(_resetButton, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (_firstRender || mode != _shownMode) {
        _shownMode = mode;
        _firstRender = false;
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
}  // namespace focus
