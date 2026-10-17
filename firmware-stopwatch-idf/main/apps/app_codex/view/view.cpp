/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "view.h"
#include "deck_layout.h"
#include "../codex_config.h"
#include <assets/assets.h>
#include <hal/hal.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string_view>

using namespace view;
using namespace uitk::lvgl_cpp;

namespace {

constexpr uint32_t kColorText    = 0xEEF2FF;
constexpr uint32_t kOwBackground = 0x05070B;
constexpr uint32_t kOwPanel      = 0x10161F;
constexpr uint32_t kOwPanelAlt   = 0x121C29;
constexpr uint32_t kOwBlue       = 0x35B8FF;
constexpr uint32_t kOwGreen      = 0x55F36A;
constexpr uint32_t kOwToday      = 0xFF7438;
constexpr uint32_t kOwAmber      = 0xFFC542;
constexpr uint32_t kOwTeal       = 0x2DF1D3;
constexpr uint32_t kOwSoftText   = 0xADB9CC;
constexpr uint32_t kOwTrack      = 0x1E2A3C;
constexpr uint32_t kOwStatusIdle = 0x7895B2;
constexpr uint32_t kOwAlert      = 0xFF405C;
constexpr uint32_t kActionWheelHoldMs = 150;
constexpr uint32_t kActionWheelFrameMs = 33;
constexpr int kActionWheelStartRadius = codex::deck::kRadialHitRadius;
constexpr int kNativeControlHoldSlop = 22;
constexpr int kRadialDetailTouchSlop = 16;
constexpr uint32_t kNativeAgentHoldMs = 280;
constexpr uint32_t kNativeAgentPreviewFrameMs = 80;
constexpr uint32_t kNativeAgentAnimationFrameMs = 80;
constexpr int kNativeAgentSwitchHysteresis = 8;
constexpr uint32_t kNativeWorkingColor = 0x304FFE;
// "CODEX LIVE" decays this long after the last quota panel arrived from the
// bridge (the bridge refreshes quota every 300 s; a closed Codex client stops
// panel pushes, so this timeout is what downgrades LIVE to BLE ONLY).
constexpr uint32_t kCodexLiveTimeoutMs = 360000;
constexpr auto kNativeAgentCenters = codex::deck::kAgentCenters;
constexpr int kScreenSize = 466;
constexpr int kScreenCenter = 233;

void setLabelTextIfChanged(Label& label, std::string_view text)
{
    const char* current = label.getText();
    if (current != nullptr && text == current) {
        return;
    }
    label.setText(text);
}

template <typename WidgetT>
void setHiddenIfChanged(WidgetT& widget, bool hidden)
{
    const bool currently_hidden = lv_obj_has_flag(widget.get(), LV_OBJ_FLAG_HIDDEN);
    if (currently_hidden != hidden) {
        widget.setHidden(hidden);
    }
}

uint32_t blendRgb(uint32_t from, uint32_t to, float amount)
{
    const float t = std::clamp(amount, 0.0f, 1.0f);
    const auto blend_channel = [t](uint32_t a, uint32_t b) {
        return static_cast<uint32_t>(std::lround(static_cast<float>(a) +
                                                 (static_cast<float>(b) - static_cast<float>(a)) * t));
    };
    const uint32_t red = blend_channel((from >> 16) & 0xFF, (to >> 16) & 0xFF);
    const uint32_t green = blend_channel((from >> 8) & 0xFF, (to >> 8) & 0xFF);
    const uint32_t blue = blend_channel(from & 0xFF, to & 0xFF);
    return (red << 16) | (green << 8) | blue;
}

constexpr bool pointInsideNativeAgentRail(int x, int y)
{
    return codex::deck::agentAt(x, y) >= 0;
}

// The same native drawing is used for the resting pad and its active feedback.
void drawRadialPad(lv_layer_t* layer, const lv_area_t& coords, int selection,
                   bool active, float hold_progress, lv_opa_t opacity)
{
    lv_draw_arc_dsc_t arc;
    lv_draw_arc_dsc_init(&arc);
    arc.center = {coords.x1 + (coords.x2 - coords.x1 + 1) / 2,
                  coords.y1 + (coords.y2 - coords.y1 + 1) / 2};
    arc.radius = codex::deck::kRadialRadius;
    arc.width = 2;
    arc.color = lv_color_hex(active ? kOwBlue : 0x287C9E);
    arc.opa = opacity;
    arc.start_angle = 0; arc.end_angle = 360;
    lv_draw_arc(layer, &arc);
    constexpr std::array<int, 4> starts = {{232, 322, 52, 142}};
    for (int direction = 0; direction < 4; ++direction) {
        const bool selected = active && selection == direction;
        arc.radius = 39; arc.width = 22; arc.rounded = 1;
        arc.start_angle = starts[direction]; arc.end_angle = starts[direction] + 76;
        arc.color = lv_color_hex(selected ? 0x246D98 : 0x14283D);
        lv_draw_arc(layer, &arc);
        arc.radius = 40; arc.width = 1;
        arc.color = lv_color_hex(selected ? kOwBlue : 0x345778);
        lv_draw_arc(layer, &arc);
        // Rotate the same chevron through up, right, down and left.
        const float angle = direction * 1.57079632679f;
        auto point = [&](int x, int y) -> lv_point_precise_t {
            return {static_cast<lv_value_precise_t>(arc.center.x + std::lround(x * std::cos(angle) - y * std::sin(angle))),
                    static_cast<lv_value_precise_t>(arc.center.y + std::lround(x * std::sin(angle) + y * std::cos(angle)))};
        };
        lv_draw_line_dsc_t line;
        lv_draw_line_dsc_init(&line);
        line.opa = opacity;
        line.color = lv_color_hex(selected ? 0x8CE8FF : kColorText);
        line.width = 3; line.round_start = 1; line.round_end = 1;
        line.p1 = point(-6, -26); line.p2 = point(0, -32); lv_draw_line(layer, &line);
        line.p1 = point(0, -32); line.p2 = point(6, -26); lv_draw_line(layer, &line);
    }
    arc.rounded = 0; arc.start_angle = 0; arc.end_angle = 360;
    for (int radius : {11, 7, 3}) {
        arc.radius = radius; arc.width = radius == 3 ? 2 : 1;
        arc.color = lv_color_hex(active ? kOwBlue : 0x287C9E);
        lv_draw_arc(layer, &arc);
    }
    if (hold_progress > 0.0f) {
        arc.radius = 46; arc.width = 2; arc.rounded = 1;
        arc.start_angle = 270; arc.end_angle = 270 + static_cast<int>(359 * hold_progress);
        arc.color = lv_color_hex(kOwBlue);
        lv_draw_arc(layer, &arc);
    }
}

void drawRadialDetail(lv_layer_t* layer, const lv_area_t& coords, int selection)
{
    const int offset_x = coords.x1 - codex::deck::kRadialDetail.x;
    const int offset_y = coords.y1 - codex::deck::kRadialDetail.y;
    constexpr std::array<int, 4> starts = {{250, 340, 70, 160}};
    for (int direction = 0; direction < 5; ++direction) {
        const auto& bounds = direction == 4 ? codex::deck::kRadialClose
                                            : codex::deck::kRadialDirections[direction];
        const lv_area_t area = {bounds.x + offset_x, bounds.y + offset_y,
                                bounds.x + offset_x + bounds.width - 1,
                                bounds.y + offset_y + bounds.height - 1};
        const auto& clip = layer->_clip_area;
        if (area.x2 < clip.x1 || area.x1 > clip.x2 || area.y2 < clip.y1 || area.y1 > clip.y2) continue;

        const bool selected = direction == selection;
        const auto& center = direction == 4 ? codex::deck::kRadialDetailCenter
                                            : codex::deck::kRadialDirectionCenters[direction];
        const int cx = center.x + offset_x, cy = center.y + offset_y;
        if (direction == 4) {
            lv_draw_rect_dsc_t rect;
            lv_draw_rect_dsc_init(&rect);
            rect.radius = 32;
            rect.bg_color = lv_color_hex(0x15191F);
            rect.bg_opa = LV_OPA_COVER;
            rect.border_width = 0;
            lv_draw_rect(layer, &rect, &area);
        } else {
            lv_draw_arc_dsc_t arc;
            lv_draw_arc_dsc_init(&arc);
            arc.center = {codex::deck::kRadialDetailCenter.x + offset_x,
                          codex::deck::kRadialDetailCenter.y + offset_y};
            arc.radius = codex::deck::kRadialArcRadius;
            arc.width = codex::deck::kRadialArcWidth;
            arc.rounded = 1;
            arc.opa = LV_OPA_COVER;
            arc.color = lv_color_hex(selected ? 0x123B45 : 0x171B22);
            arc.start_angle = starts[direction]; arc.end_angle = starts[direction] + 40;
            lv_draw_arc(layer, &arc);
        }

        lv_draw_line_dsc_t line;
        lv_draw_line_dsc_init(&line);
        line.color = lv_color_hex(direction == 4 ? 0x9DA9B6 : (selected ? 0x62D8E8 : 0xE9EEF5));
        line.width = direction == 4 ? 2 : 4;
        line.round_start = 1; line.round_end = 1;
        if (direction == 4) {
            line.p1 = {cx - 7, cy - 7}; line.p2 = {cx + 7, cy + 7}; lv_draw_line(layer, &line);
            line.p1 = {cx + 7, cy - 7}; line.p2 = {cx - 7, cy + 7}; lv_draw_line(layer, &line);
        } else {
            const float angle = direction * 1.57079632679f;
            auto point = [&](int x, int y) -> lv_point_precise_t {
                return {static_cast<lv_value_precise_t>(cx + std::lround(x * std::cos(angle) - y * std::sin(angle))),
                        static_cast<lv_value_precise_t>(cy + std::lround(x * std::sin(angle) + y * std::cos(angle)))};
            };
            line.p1 = point(-13, 6); line.p2 = point(0, -8); lv_draw_line(layer, &line);
            line.p1 = point(0, -8); line.p2 = point(13, 6); lv_draw_line(layer, &line);
        }
    }
}

float clamp01(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

uint32_t blend_color(uint32_t from, uint32_t to, float amount)
{
    const float t = clamp01(amount);
    const uint8_t fr = static_cast<uint8_t>((from >> 16) & 0xFF);
    const uint8_t fg = static_cast<uint8_t>((from >> 8) & 0xFF);
    const uint8_t fb = static_cast<uint8_t>(from & 0xFF);
    const uint8_t tr = static_cast<uint8_t>((to >> 16) & 0xFF);
    const uint8_t tg = static_cast<uint8_t>((to >> 8) & 0xFF);
    const uint8_t tb = static_cast<uint8_t>(to & 0xFF);
    const auto mix = [t](uint8_t a, uint8_t b) -> uint8_t {
        return static_cast<uint8_t>(std::lround(static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * t));
    };
    return (static_cast<uint32_t>(mix(fr, tr)) << 16) |
           (static_cast<uint32_t>(mix(fg, tg)) << 8) |
           static_cast<uint32_t>(mix(fb, tb));
}

uint32_t quota_semantic_color(float fraction)
{
    const float value = clamp01(fraction);
    if (value < 0.25f) {
        return blend_color(0xFF4A3A, 0xFF9F19, value / 0.25f);
    }
    if (value < 0.50f) {
        return blend_color(0xFF9F19, 0xFFD43B, (value - 0.25f) / 0.25f);
    }
    return blend_color(0xFFD43B, kOwGreen, clamp01((value - 0.50f) / 0.30f));
}

uint32_t activity_heat_color(float activity)
{
    const float value = clamp01(activity);
    if (value <= 0.001f) {
        return 0x142236;
    }
    if (value < 0.10f) {
        return 0x18364A;
    }
    if (value < 0.25f) {
        return 0x174B63;
    }
    if (value < 0.45f) {
        return 0x176078;
    }
    if (value < 0.65f) {
        return 0x1591A2;
    }
    if (value < 0.82f) {
        return 0x18B7B6;
    }
    return kOwTeal;
}

}  // namespace

void CodexView::init(lv_obj_t* parent)
{
    lv_obj_remove_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(parent,
                              lv_color_hex(kOwBackground),
                              LV_PART_MAIN);

    _state.weekly   = {1.0f, 1.0f, "--"};
    _state.message  = "Waiting...";

    _panel = std::make_unique<Container>(parent);
    _panel->align(LV_ALIGN_CENTER, 0, 0);
    _panel->setSize(466, 466);
    _panel->setRadius(0);
    _panel->setBorderWidth(0);
    _panel->setPaddingAll(0);
    _panel->setBgColor(lv_color_hex(kOwBackground));
    _panel->setBgOpa(LV_OPA_COVER);
    _panel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    initOpenWatcherV2();

    _message_label = std::make_unique<Label>(_panel->get());
    _message_label->setText(_state.message.c_str());
    _message_label->setTextFont(&lv_font_montserrat_16);
    _message_label->setTextColor(lv_color_hex(kOwSoftText));
    _message_label->setWidth(180);
    _message_label->setLongMode(LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    _message_label->setTextAlign(LV_TEXT_ALIGN_CENTER);
    _message_label->align(LV_ALIGN_TOP_MID,
                          0,
                          425);
    if (_message_label) {
        _message_label->setHidden(true);
    }

    initVoiceWaveform();

    initStatusIndicators();

    initActionWheel();
    initTaskList();

    updateStatusIndicators();
    updateOpenWatcherV2Labels();
    GetHAL().updateImuData();
}

void CodexView::update()
{
    updateEncoderFeedbackTimeout();
    updateStatusIndicators();
    if (_task_list_active) {
        return;
    }
    updateEncoderTouchControl();
    if (_encoder_touch_tracking) {
        return;
    }
    if (_native_agent_touch_tracking) {
        return;
    }
    updateActionWheelGesture();
    if (_action_wheel_active) {
        return;
    }

    const uint32_t now = GetHAL().millis();
    updateNativeAgentRail(false);
    if (_state.messageExpiresAtMs != 0 && now > _state.messageExpiresAtMs) {
        _state.messageExpiresAtMs = 0;
        _state.message.clear();
        updateOpenWatcherV2Labels();
        if (_message_label) {
            _message_label->setText("");
            _message_label->setHidden(true);
        }
    }

    updateMotionInput();

    updateVoiceWaveform();
}

void CodexView::applyQuota(int weeklyLeftPct,
                           const std::string& weeklyResetLabel,
                           bool valid,
                           bool wifiConnected,
                           bool processing,
                           const std::string& message)
{
    codex::QuotaSnapshot snapshot;
    snapshot.weeklyLeftPct = weeklyLeftPct;
    snapshot.weeklyUsage = weeklyResetLabel;
    snapshot.valid = valid;
    snapshot.wifiConnected = wifiConnected;
    snapshot.processing = processing;
    snapshot.message = message;
    applySnapshot(snapshot);
}

void CodexView::applySnapshot(const codex::QuotaSnapshot& snapshot)
{
    const int previous_week_pct = static_cast<int>(std::lround(remainingRatio(_state.weekly) * 100.0f));
    const int previous_session_pct = _state.session5hLeftPct;
    const bool previous_stale = _state.stale;
    const bool previous_quota_valid = _state.quotaValid;
    const auto previous_activity_buckets = _state.activityBuckets;

    _state.wifiConnected = snapshot.wifiConnected;
    _state.processing = snapshot.processing;
    _state.source = snapshot.source;
    _state.updatedAt = snapshot.updatedAt;
    _state.stale = snapshot.stale || snapshot.cached;
    _state.quotaValid = snapshot.valid;
    _state.hostName = snapshot.hostName;
    _state.sessionTitle = snapshot.sessionTitle.empty() ? "Codex Ready" : snapshot.sessionTitle;
    _state.contextLabel = snapshot.contextLabel.empty() ? "-- / --" : snapshot.contextLabel;
    _state.contextPressurePct = std::clamp(snapshot.contextPressurePct, 0, 100);
    _state.compactThresholdPct = snapshot.compactThresholdPct;
    _state.compactWarning = snapshot.compactWarning;
    _state.totalTokensLabel = snapshot.totalTokensLabel.empty() ? "--" : snapshot.totalTokensLabel;
    _state.modelLabel = snapshot.modelLabel.empty() ? "--" : snapshot.modelLabel;
    _state.reasoningLabel = snapshot.reasoningLabel.empty() ? "--" : snapshot.reasoningLabel;
    _state.activityLabel = snapshot.activityLabel.empty() ? "--" : snapshot.activityLabel;
    _state.activityLive = snapshot.activityLive;
    _state.activityBuckets = snapshot.activityBuckets;
    _state.weeklyDayStartLeftPct = snapshot.weeklyDayStartLeftPct;
    _state.weeklySegmentStartLeftPct = snapshot.weeklySegmentStartLeftPct;
    _state.weeklyTodayUsedPctPoints = snapshot.weeklyTodayUsedPctPoints;
    _state.weeklyResetCount = snapshot.weeklyResetCount;
    _state.weeklyTrackingPeriodStart = snapshot.weeklyTrackingPeriodStart;
    _last_quota_update_tick = GetHAL().millis();

    if (snapshot.valid) {
        _state.weekly.used = 100.0f - static_cast<float>(std::clamp(snapshot.weeklyLeftPct, 0, 100));
        _state.weekly.limit = 100.0f;
        _state.weekly.resetInLabel = snapshot.weeklyUsage.empty() ? "--" : snapshot.weeklyUsage;
        _state.session5hLeftPct = snapshot.sessionLeftPct >= 0 && snapshot.sessionLeftPct <= 100
                                     ? snapshot.sessionLeftPct : -1;
        _state.session5hResetLabel = (_state.session5hLeftPct >= 0 && !snapshot.sessionResetIn.empty())
                                         ? snapshot.sessionResetIn
                                         : "--";
    } else {
        _state.weekly.used = 100.0f;
        _state.weekly.limit = 100.0f;
        _state.weekly.resetInLabel = "--";
        _state.session5hLeftPct = -1;
        _state.session5hResetLabel = "--";
    }

    const int updated_week_pct = static_cast<int>(std::lround(remainingRatio(_state.weekly) * 100.0f));
    const bool v2_canvas_changed = previous_week_pct != updated_week_pct ||
                                   previous_session_pct != _state.session5hLeftPct ||
                                   previous_stale != _state.stale ||
                                   previous_quota_valid != _state.quotaValid ||
                                   previous_activity_buckets != _state.activityBuckets;

    updateStatusIndicators();
    updateOpenWatcherV2Labels();
    if (v2_canvas_changed && _v2_canvas) {
        lv_obj_invalidate(_v2_canvas->get());
    }
    setMessageText(snapshot.message);
}

void CodexView::applyBleState(bool connected, const std::string& hostMessage, bool hostMessageChanged)
{
    if (_state.bleConnected != connected) {
        _state.bleConnected = connected;
        updateStatusIndicators();
        updateOpenWatcherV2Labels();
    }

    if (hostMessageChanged && !hostMessage.empty()) {
        if (hostMessage == "Typeless recording" ||
            hostMessage == "Typeless processing" ||
            hostMessage == "Typeless idle" ||
            hostMessage == "Bridge ready") {
            if (hostMessage == "Bridge ready" &&
                (_state.message == "Bridge unavailable" ||
                 _state.message == "App unavailable" ||
                 _state.message == "Host state unknown")) {
                setMessageText("Ready");
            }
            return;
        }
        std::string message = hostMessage;
        if (hostMessage == "BLE connected" || hostMessage == "BLE paired" ||
            hostMessage == "BLE disconnected" || hostMessage == "BLE ready") {
            // The icon already reflects this transition; keep it visible.
            return;
        } else if (hostMessage == "Key sent") {
            return;
        } else if (hostMessage == "Key send failed") {
            return;
        } else if (hostMessage == "Typeless request sent") {
            return;
        } else if (hostMessage == "Bridge fallback key") {
            return;
        }
        setMessageText(message);
    }
}

void CodexView::setUnreadTaskCount(int count)
{
    _state.unreadStateValid = true;
    _state.unreadTaskCount = std::max(count, 0);
    updateOpenWatcherV2Labels();
}

void CodexView::setUnreadTasks(const std::vector<TaskItem>& tasks)
{
    _unread_tasks = tasks;
    updateTaskListRows();
}

void CodexView::setNativeAgentStates(bool ready,
                                     bool communicating,
                                     const std::array<NativeAgentState, 6>& agents)
{
    _native_agents_ready = ready;
    _native_codex_communicating = communicating;
    _native_agents = agents;
    updateNativeAgentRail(true);
    updateOpenWatcherV2Labels();
}

void CodexView::showTaskList()
{
    if (_unread_tasks.empty()) {
        showActionMessage("NO UNREAD TASKS");
        return;
    }
    setTaskListVisible(true);
}

std::string CodexView::consumeTaskOpenRequest()
{
    std::string request = _task_open_request;
    _task_open_request.clear();
    return request;
}

int CodexView::consumeNativeAgentSlotRequest()
{
    const int request = _native_agent_slot_request;
    _native_agent_slot_request = -1;
    return request;
}

int CodexView::consumeNativeActionRequest()
{
    const int request = _native_action_request;
    _native_action_request = -1;
    return request;
}

void CodexView::showActionMessage(const char* message)
{
    setMessage(message, 1800);
}

void CodexView::setVoiceActive(bool active)
{
    setVoiceMode(active ? VoiceMode::Recording : VoiceMode::Idle);
}

void CodexView::setVoiceMode(VoiceMode mode)
{
    if (_voice_mode == mode) {
        return;
    }

    _voice_mode = mode;
    if (mode != VoiceMode::Idle && _native_agent_touch_tracking) {
        resetNativeAgentTouch();
    }
    if (mode != VoiceMode::Idle && (_action_wheel_active || _action_touch_tracking)) {
        resetNativeTouchControl(true);
        _native_control_blocked_until_release = true;
    }
    if (mode != VoiceMode::Idle && _task_list_active) {
        setTaskListVisible(false);
    }
    if (mode != VoiceMode::Idle && _encoder_touch_tracking) {
        _encoder_touch_cancelled = true;
        _encoder_touch_tracking = false;
        _encoder_pressed_target = -1;
        lv_obj_invalidate(_v2_knob->get());
    }
    // Capture ends before recognition. Do not keep the recording animation
    // alive while waiting for the host's Processing -> Idle acknowledgement.
    const bool visible = mode == VoiceMode::Recording;
    if (_voice_waveform) {
        _voice_waveform->setHidden(!visible);
    }
    if (mode == VoiceMode::Interrupted) {
        _state.message = "MIC LOST - PRESS A";
        _state.messageExpiresAtMs = 0;
        updateOpenWatcherV2Labels();
        if (_message_label) {
            _message_label->setText(_state.message.c_str());
            _message_label->setTextColor(lv_color_hex(kOwAmber));
            _message_label->setHidden(true);
        }
    } else {
        if (_message_label) {
            if (_state.message == "MIC LOST - PRESS A") {
                _state.message.clear();
                _message_label->setText("");
            }
            _message_label->setTextColor(lv_color_hex(kOwSoftText));
            _message_label->setHidden(mode != VoiceMode::Idle);
        }
    }
    updateOpenWatcherV2Labels();
    if (_action_wheel_canvas) {
        setHiddenIfChanged(*_action_wheel_canvas,
                           mode == VoiceMode::Recording || mode == VoiceMode::Interrupted);
        lv_obj_set_style_opa(_action_wheel_canvas->get(), mode == VoiceMode::Processing ? 100 : 255, 0);
        lv_obj_invalidate(_action_wheel_canvas->get());
    }
}

uint32_t CodexView::frameIntervalMs() const
{
    // Poll reasoning and center gestures every frame, including the first
    // touch. Widgets still use differential updates to avoid full repaints.
    return _task_list_active ? 100 : kActionWheelFrameMs;
}

bool CodexView::hasExecutingNativeAgent() const
{
    if (!_native_agents_ready) {
        return false;
    }
    const size_t visible_slots = std::min(kNativeAgentCenters.size(), _native_agents.size());
    for (size_t index = 0; index < visible_slots; ++index) {
        const auto& agent = _native_agents[index];
        if (agent.assigned && agent.color == kNativeWorkingColor && agent.brightness > 0.01f) {
            return true;
        }
    }
    return false;
}

bool CodexView::consumeClearInputRequest()
{
    if (!_clear_input_requested) {
        return false;
    }
    _clear_input_requested = false;
    return true;
}

int CodexView::consumeReasoningDeltaRequest()
{
    const int request = _reasoning_delta_request;
    _reasoning_delta_request = 0;
    return request;
}

bool CodexView::consumeNativeRadialRequest(float& angle, float& distance)
{
    if (!_native_radial_pending) {
        return false;
    }
    angle = _native_radial_angle;
    distance = _native_radial_distance;
    _native_radial_pending = false;
    return true;
}

void CodexView::initActionWheel()
{
    _action_wheel_canvas = std::make_unique<Container>(_panel->get());
    _action_wheel_canvas->setSize(100, 100);
    _action_wheel_canvas->align(LV_ALIGN_TOP_LEFT,
        codex::deck::kRadialCenter.x - 50, codex::deck::kRadialCenter.y - 50);
    _action_wheel_canvas->setBgOpa(LV_OPA_TRANSP);
    _action_wheel_canvas->setBorderWidth(0);
    _action_wheel_canvas->setPaddingAll(0);
    _action_wheel_canvas->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
    _action_wheel_canvas->removeFlag(LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(_action_wheel_canvas->get(), &CodexView::drawActionWheelEvent,
                        LV_EVENT_DRAW_MAIN, this);
    // Keep both surfaces at fixed bounds. Moving a widget to the foreground
    // invalidates its entire parent in LVGL, so reserve the top layer once.
    _radial_detail_canvas = std::make_unique<Container>(_panel->get());
    const auto& detail = codex::deck::kRadialDetail;
    _radial_detail_canvas->setSize(detail.width, detail.height);
    _radial_detail_canvas->align(LV_ALIGN_TOP_LEFT, detail.x, detail.y);
    _radial_detail_canvas->setBgColor(lv_color_hex(0x0B0D11));
    _radial_detail_canvas->setBgOpa(LV_OPA_COVER);
    _radial_detail_canvas->setRadius(codex::deck::kRadialDetailRadius);
    _radial_detail_canvas->setBorderWidth(0);
    _radial_detail_canvas->setPaddingAll(0);
    _radial_detail_canvas->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
    _radial_detail_canvas->addFlag(LV_OBJ_FLAG_CLICKABLE);
    _radial_detail_canvas->setHidden(true);
    lv_obj_add_event_cb(_radial_detail_canvas->get(), &CodexView::drawActionWheelEvent,
                        LV_EVENT_DRAW_MAIN, this);
    lv_obj_add_event_cb(_radial_detail_canvas->get(), &CodexView::radialDetailTouchEvent,
                        LV_EVENT_PRESSED, this);
}

void CodexView::setActionWheelVisible(bool visible)
{
    if (!_action_wheel_canvas || !_radial_detail_canvas || _action_wheel_active == visible) {
        return;
    }
    _action_wheel_active = visible;
    _action_wheel_selection = -1;
    setHiddenIfChanged(*_radial_detail_canvas, !visible);
    setHiddenIfChanged(*_action_wheel_canvas,
                       visible || _voice_mode == VoiceMode::Recording || _voice_mode == VoiceMode::Interrupted);
}

void CodexView::updateActionWheelSelection(int selection)
{
    selection = (selection >= 0 && selection <= 4) ? selection : -1;
    if (_action_wheel_selection == selection) {
        return;
    }
    const int previous = _action_wheel_selection;
    _action_wheel_selection = selection;
    if (_radial_detail_canvas && _action_wheel_active) {
        // Feedback changes only these button surfaces, not the 304px panel.
        for (int target : {previous, selection}) {
            if (target < 0) continue;
            const auto& rect = target == 4 ? codex::deck::kRadialClose
                                          : codex::deck::kRadialDirections[target];
            const lv_area_t area = {rect.x, rect.y, rect.x + rect.width - 1, rect.y + rect.height - 1};
            lv_obj_invalidate_area(_radial_detail_canvas->get(), &area);
        }
    }
}

void CodexView::queueNativeRadial(float angle, float distance)
{
    _native_radial_angle = std::clamp(angle, 0.0f, 1.0f);
    _native_radial_distance = std::clamp(distance, 0.0f, 1.0f);
    _native_radial_pending = true;
}

void CodexView::resetNativeTouchControl(bool queueNeutral)
{
    if (queueNeutral && _native_radial_distance > 0.0f) {
        queueNativeRadial(_native_radial_angle, 0.0f);
    }
    setActionWheelVisible(false);
    _action_touch_tracking = false;
    _action_touch_started_at = 0;
    _action_touch_start_x = 0;
    _action_touch_start_y = 0;
    _radial_detail_touch_target = -1;
    updateActionWheelSelection(-1);
    if (_action_wheel_canvas) lv_obj_invalidate(_action_wheel_canvas->get());
}

void CodexView::updateActionWheelGesture()
{
    if (_task_list_active) return;
    lv_indev_t* indev = GetHAL().lvTouchpad;
    if (!indev) return;

    const uint32_t now = GetHAL().millis();
    const bool pressed = lv_indev_get_state(indev) == LV_INDEV_STATE_PR;
    lv_point_t point = {0, 0};
    lv_indev_get_point(indev, &point);

    // Opening the detail panel never selects a direction. Require a fresh
    // contact after release, including when its new bounds cover that finger.
    if (_native_control_blocked_until_release) {
        if (!pressed) _native_control_blocked_until_release = false;
        return;
    }
    if (_voice_mode != VoiceMode::Idle) return;

    if (_action_wheel_active) {
        const int target = codex::deck::radialDetailAt(point.x, point.y);
        if (pressed) {
            if (!_action_touch_tracking) {
                _action_touch_tracking = true;
                _action_touch_start_x = point.x;
                _action_touch_start_y = point.y;
                _radial_detail_touch_target = target;
                if (target == 4) {
                    // Closing is safe at press time. Consume this contact
                    // through release so it cannot reach the controls below.
                    setActionWheelVisible(false);
                    _action_touch_tracking = false;
                    _radial_detail_touch_target = -1;
                    _native_control_blocked_until_release = true;
                    return;
                }
                updateActionWheelSelection(target);
                if (target >= 0 && target < 4) {
                    constexpr float angles[] = {0.75f, 0.0f, 0.25f, 0.5f};
                    queueNativeRadial(angles[target], 1.0f);
                    GetHAL().vibrate(18, 45);
                }
            } else {
                const int dx = point.x - _action_touch_start_x;
                const int dy = point.y - _action_touch_start_y;
                if (target != _radial_detail_touch_target ||
                    dx * dx + dy * dy > kRadialDetailTouchSlop * kRadialDetailTouchSlop) {
                    if (_native_radial_distance > 0.0f) queueNativeRadial(_native_radial_angle, 0.0f);
                    _action_touch_tracking = false;
                    _radial_detail_touch_target = -1;
                    updateActionWheelSelection(-1);
                    _native_control_blocked_until_release = true;
                }
            }
            return;
        }
        if (_action_touch_tracking) {
            if (_native_radial_distance > 0.0f) queueNativeRadial(_native_radial_angle, 0.0f);
            const bool close = target == _radial_detail_touch_target && (target == 4 || target == 5);
            _action_touch_tracking = false;
            _radial_detail_touch_target = -1;
            updateActionWheelSelection(-1);
            if (close) setActionWheelVisible(false);
        }
        return;
    }

    if (!pressed) {
        if (_action_touch_tracking) resetNativeTouchControl(false);
        return;
    }
    if (!_action_touch_tracking) {
        if (pointInsideNativeAgentRail(point.x, point.y)) return;
        if (!codex::deck::insideRadial(point.x, point.y)) {
            _native_control_blocked_until_release = true;
            return;
        }
        _action_touch_tracking = true;
        _action_touch_started_at = now;
        _action_touch_start_x = point.x;
        _action_touch_start_y = point.y;
        GetHAL().vibrate(14, 35);
        lv_obj_invalidate(_action_wheel_canvas->get());
        return;
    }
    const int dx = point.x - _action_touch_start_x;
    const int dy = point.y - _action_touch_start_y;
    if (dx * dx + dy * dy > kNativeControlHoldSlop * kNativeControlHoldSlop) {
        resetNativeTouchControl(false);
        _native_control_blocked_until_release = true;
        return;
    }
    if (now - _action_touch_started_at >= kActionWheelHoldMs) {
        setActionWheelVisible(true);
        _action_touch_tracking = false;
        _native_control_blocked_until_release = true;
        GetHAL().vibrate(35, 80);
        return;
    }
}

void CodexView::radialDetailTouchEvent(lv_event_t* event)
{
    auto* view = static_cast<CodexView*>(lv_event_get_user_data(event));
    if (!view || !view->_action_wheel_active || view->_native_control_blocked_until_release ||
        view->_voice_mode != VoiceMode::Idle) return;
    lv_indev_t* indev = lv_event_get_indev(event);
    if (!indev) return;
    lv_point_t point;
    lv_indev_get_point(indev, &point);
    if (codex::deck::radialDetailAt(point.x, point.y) != 4) return;
    view->setActionWheelVisible(false);
    view->_action_touch_tracking = false;
    view->_radial_detail_touch_target = -1;
    view->_native_control_blocked_until_release = true;
}

void CodexView::drawActionWheelEvent(lv_event_t* event)
{
    auto* view = static_cast<CodexView*>(lv_event_get_user_data(event));
    if (view == nullptr || lv_event_get_code(event) != LV_EVENT_DRAW_MAIN) {
        return;
    }
    lv_obj_t* obj = lv_event_get_target_obj(event);
    lv_layer_t* layer = lv_event_get_layer(event);
    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);
    if (obj == view->_radial_detail_canvas->get()) drawRadialDetail(layer, coords, view->_action_wheel_selection);
    else view->drawActionWheel(layer, coords);
}

void CodexView::drawActionWheel(lv_layer_t* layer, const lv_area_t& coords)
{
    // Immediate contact feedback, with no per-frame animation competing with
    // the 150ms recognition timer for the display lock.
    const float hold = _action_touch_tracking && !_action_wheel_active ? 1.0f : 0.0f;
    drawRadialPad(layer, coords, _action_wheel_selection, _action_wheel_active, hold,
                  lv_obj_get_style_opa_recursive(_action_wheel_canvas->get(), LV_PART_MAIN));
}

void CodexView::initTaskList()
{
    _task_list_canvas = std::make_unique<Container>(_panel->get());
    _task_list_canvas->setSize(kScreenSize, kScreenSize);
    _task_list_canvas->align(LV_ALIGN_CENTER, 0, 0);
    _task_list_canvas->setBgColor(lv_color_hex(0x02050A));
    _task_list_canvas->setBgOpa(252);
    _task_list_canvas->setBorderWidth(0);
    _task_list_canvas->setPaddingAll(0);
    _task_list_canvas->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    _task_list_title = std::make_unique<Label>(_task_list_canvas->get());
    _task_list_title->setText("UNREAD TASKS");
    _task_list_title->setTextFont(&lv_font_montserrat_22);
    _task_list_title->setTextColor(lv_color_hex(kOwBlue));
    _task_list_title->setWidth(280);
    _task_list_title->setTextAlign(LV_TEXT_ALIGN_CENTER);
    _task_list_title->align(LV_ALIGN_TOP_MID, 0, 58);

    for (size_t i = 0; i < _task_row_buttons.size(); ++i) {
        auto row = std::make_unique<Container>(_task_list_canvas->get());
        row->setSize(344, 42);
        row->align(LV_ALIGN_TOP_MID, 0, 102 + static_cast<int>(i) * 48);
        row->setRadius(15);
        row->setBgColor(lv_color_hex(i == 0 ? 0x102B43 : kOwPanel));
        row->setBgOpa(245);
        row->setBorderWidth(1);
        row->setBorderColor(lv_color_hex(i == 0 ? kOwBlue : 0x26374C));
        row->setPaddingAll(0);
        row->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
        row->onClick().connect([this, i]() {
            if (i >= _unread_tasks.size()) {
                return;
            }
            _task_open_request = _unread_tasks[i].id;
            setTaskListVisible(false);
            GetHAL().vibrate(45, 85);
        });

        auto label = std::make_unique<Label>(row->get());
        label->setText("");
#if CONFIG_LV_FONT_SOURCE_HAN_SANS_SC_16_CJK
        label->setTextFont(&lv_font_source_han_sans_sc_16_cjk);
#else
        label->setTextFont(&lv_font_montserrat_16);
#endif
        label->setTextColor(lv_color_hex(kColorText));
        label->setWidth(304);
        label->setTextAlign(LV_TEXT_ALIGN_LEFT);
        label->align(LV_ALIGN_LEFT_MID, 16, 0);
        lv_label_set_long_mode(label->get(), LV_LABEL_LONG_MODE_DOTS);
        label->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
        _task_row_buttons[i] = std::move(row);
        _task_row_labels[i] = std::move(label);
    }

    _task_list_back_button = std::make_unique<Container>(_task_list_canvas->get());
    _task_list_back_button->setSize(132, 38);
    _task_list_back_button->align(LV_ALIGN_BOTTOM_MID, 0, -40);
    _task_list_back_button->setRadius(19);
    _task_list_back_button->setBgColor(lv_color_hex(0x121C29));
    _task_list_back_button->setBgOpa(250);
    _task_list_back_button->setBorderWidth(1);
    _task_list_back_button->setBorderColor(lv_color_hex(0x39506B));
    _task_list_back_button->setPaddingAll(0);
    _task_list_back_button->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
    _task_list_back_button->onClick().connect([this]() {
        setTaskListVisible(false);
        GetHAL().vibrate(25, 55);
    });

    _task_list_back_label = std::make_unique<Label>(_task_list_back_button->get());
    _task_list_back_label->setText("BACK");
    _task_list_back_label->setTextFont(&lv_font_montserrat_16);
    _task_list_back_label->setTextColor(lv_color_hex(kOwSoftText));
    _task_list_back_label->align(LV_ALIGN_CENTER, 0, 0);

    updateTaskListRows();
    setTaskListVisible(false);
}

void CodexView::setTaskListVisible(bool visible)
{
    if (!_task_list_canvas) {
        return;
    }
    _task_list_active = visible;
    _task_list_canvas->setHidden(!visible);
    if (visible) {
        updateTaskListRows();
        _task_list_canvas->moveForeground();
    }
}

void CodexView::updateTaskListRows()
{
    for (size_t i = 0; i < _task_row_buttons.size(); ++i) {
        if (!_task_row_buttons[i] || !_task_row_labels[i]) {
            continue;
        }
        const bool available = i < _unread_tasks.size();
        _task_row_buttons[i]->setHidden(!available);
        if (!available) {
            continue;
        }
        char prefix[8] = {};
        std::snprintf(prefix, sizeof(prefix), "%u  ", static_cast<unsigned>(i + 1));
        _task_row_labels[i]->setText((std::string(prefix) + _unread_tasks[i].title).c_str());
    }
}

void CodexView::updateEncoderTouchControl()
{
    lv_indev_t* indev = GetHAL().lvTouchpad;
    if (!indev || _task_list_active || _action_wheel_active) return;
    lv_point_t point{};
    lv_indev_get_point(indev, &point);
    const bool down = lv_indev_get_state(indev) == LV_INDEV_STATE_PR;
    if (down) {
        if (!_encoder_touch_down) {
            _encoder_touch_down = true;
            _encoder_pressed_target = _voice_mode == VoiceMode::Idle
                ? codex::deck::encoderAt(point.x, point.y) : -1;
            _encoder_touch_tracking = _encoder_pressed_target >= 0;
            _encoder_touch_cancelled = false;
            _encoder_touch_start_x = point.x;
            _encoder_touch_start_y = point.y;
            if (_encoder_touch_tracking) {
                GetHAL().vibrate(12, 35);
                showEncoderFeedback(_encoder_pressed_target);
                lv_obj_invalidate(_v2_knob->get());
            }
        } else if (_encoder_touch_tracking && !_encoder_touch_cancelled) {
            const int dx = point.x - _encoder_touch_start_x;
            const int dy = point.y - _encoder_touch_start_y;
            if (_voice_mode != VoiceMode::Idle ||
                codex::deck::encoderAt(point.x, point.y) != _encoder_pressed_target ||
                dx * dx + dy * dy > 16 * 16) {
                _encoder_touch_cancelled = true;
                lv_obj_invalidate(_v2_knob->get());
            }
        }
        return;
    }
    if (!_encoder_touch_down) return;
    if (_encoder_touch_tracking && !_encoder_touch_cancelled &&
        _voice_mode == VoiceMode::Idle &&
        codex::deck::encoderAt(point.x, point.y) == _encoder_pressed_target) {
        _reasoning_delta_request += _encoder_pressed_target == 0 ? -1 : 1;
        GetHAL().vibrate(18, 45);
    }
    _encoder_touch_down = false;
    _encoder_touch_tracking = false;
    _encoder_touch_cancelled = false;
    _encoder_pressed_target = -1;
    lv_obj_invalidate(_v2_knob->get());
}

void CodexView::showEncoderFeedback(int side)
{
    if (side < 0 || side >= 2) return;
    setLabelTextIfChanged(*_v2_encoder_feedback_labels[0], side == 0 ? "-1" : "");
    setLabelTextIfChanged(*_v2_encoder_feedback_labels[1], side == 1 ? "+1" : "");
    _encoder_feedback_side = side;
    _encoder_feedback_expires_at = GetHAL().millis() + 500;
}

void CodexView::updateEncoderFeedbackTimeout()
{
    if (_encoder_feedback_side < 0 ||
        static_cast<int32_t>(GetHAL().millis() - _encoder_feedback_expires_at) < 0) return;
    setLabelTextIfChanged(*_v2_encoder_feedback_labels[0], "");
    setLabelTextIfChanged(*_v2_encoder_feedback_labels[1], "");
    _encoder_feedback_side = -1;
    _encoder_feedback_expires_at = 0;
}

void CodexView::drawEncoderButtonsEvent(lv_event_t* event)
{
    auto* view = static_cast<CodexView*>(lv_event_get_user_data(event));
    if (!view || lv_event_get_code(event) != LV_EVENT_DRAW_MAIN_END) return;
    lv_area_t coords;
    lv_obj_get_coords(lv_event_get_target_obj(event), &coords);
    view->drawEncoderButtons(lv_event_get_layer(event), coords);
}

void CodexView::drawEncoderButtons(lv_layer_t* layer, const lv_area_t& coords)
{
    for (int side = 0; side < 2; ++side) {
        const bool pressed = _encoder_touch_tracking && !_encoder_touch_cancelled &&
                             _encoder_pressed_target == side;
        const int x = coords.x1 + side * 100;
        lv_area_t area{x, coords.y1, x + 83, coords.y2};
        lv_draw_rect_dsc_t rect;
        lv_draw_rect_dsc_init(&rect);
        rect.radius = 17; rect.bg_opa = 255;
        rect.bg_color = lv_color_hex(pressed ? 0x12374D : 0x0B1726);
        rect.border_width = 1;
        rect.border_color = lv_color_hex(pressed ? kOwBlue : 0x278EAF);
        lv_draw_rect(layer, &rect, &area);
        const int center_x = x + 42, center_y = coords.y1 + 17;
        const int sign = side == 0 ? -1 : 1;
        lv_draw_line_dsc_t line;
        lv_draw_line_dsc_init(&line);
        line.width = 3; line.round_start = 1; line.round_end = 1;
        line.color = lv_color_hex(pressed ? 0x89E5FF : kColorText);
        line.p1 = {static_cast<lv_value_precise_t>(center_x - sign * 4), static_cast<lv_value_precise_t>(center_y - 6)};
        line.p2 = {static_cast<lv_value_precise_t>(center_x + sign * 3), static_cast<lv_value_precise_t>(center_y)};
        lv_draw_line(layer, &line);
        line.p1 = line.p2;
        line.p2 = {static_cast<lv_value_precise_t>(center_x - sign * 4), static_cast<lv_value_precise_t>(center_y + 6)};
        lv_draw_line(layer, &line);
    }
}

bool CodexView::codexLiveActive() const
{
    // A live Bridge alone does not establish that the Codex client is live.
    if (!_native_codex_communicating || _state.stale || _last_quota_update_tick == 0) {
        return false;
    }
    return (GetHAL().millis() - _last_quota_update_tick) < kCodexLiveTimeoutMs;
}

void CodexView::initOpenWatcherV2()
{
    _v2_canvas = std::make_unique<Container>(_panel->get());
    _v2_canvas->setSize(466, 258);
    _v2_canvas->align(LV_ALIGN_TOP_LEFT, 0, 0);
    _v2_canvas->setBgOpa(LV_OPA_TRANSP);
    _v2_canvas->setBorderWidth(0);
    _v2_canvas->setPaddingAll(0);
    _v2_canvas->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
    _v2_canvas->removeFlag(LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(_v2_canvas->get(), &CodexView::drawOpenWatcherV2Event, LV_EVENT_DRAW_MAIN_BEGIN, this);

    auto label = [&](const char* text, const lv_font_t* font, int x, int y, int width,
                     uint32_t color, lv_text_align_t align = LV_TEXT_ALIGN_CENTER) {
        auto item = std::make_unique<Label>(_panel->get());
        item->setText(text); item->setTextFont(font); item->setTextColor(lv_color_hex(color));
        item->setWidth(width); item->setTextAlign(align);
        item->align(LV_ALIGN_TOP_LEFT, x, y);
        item->removeFlag(LV_OBJ_FLAG_CLICKABLE);
        return item;
    };
    _v2_title_label = label("Codex Offline", &lv_font_montserrat_20, 93, 26, 280, kOwBlue);
    _v2_title_label->setLongMode(LV_LABEL_LONG_MODE_DOTS);
    _v2_remaining_group = std::make_unique<Container>(_panel->get());
    _v2_remaining_group->setSize(300, 66);
    _v2_remaining_group->align(LV_ALIGN_TOP_LEFT, 83, 48);
    _v2_remaining_group->setBgOpa(LV_OPA_TRANSP);
    _v2_remaining_group->setBorderWidth(0);
    _v2_remaining_group->setPaddingAll(0);
    _v2_remaining_group->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
    _v2_remaining_group->removeFlag(LV_OBJ_FLAG_CLICKABLE);
    _v2_remaining_value_label = std::make_unique<Label>(_v2_remaining_group->get());
    _v2_remaining_value_label->setText("-");
    _v2_remaining_value_label->setTextFont(&CommissionerMedium64);
    _v2_remaining_value_label->setTextColor(lv_color_hex(kOwGreen));
    _v2_remaining_value_label->setSize(150, LV_SIZE_CONTENT);
    _v2_remaining_value_label->setTextAlign(LV_TEXT_ALIGN_RIGHT);
    _v2_remaining_value_label->align(LV_ALIGN_BOTTOM_LEFT, 0, 0);
    _v2_remaining_value_label->removeFlag(LV_OBJ_FLAG_CLICKABLE);
    _v2_remaining_ones_label = std::make_unique<Label>(_v2_remaining_group->get());
    _v2_remaining_ones_label->setText("-");
    _v2_remaining_ones_label->setTextFont(&CommissionerMedium64);
    _v2_remaining_ones_label->setTextColor(lv_color_hex(kOwGreen));
    _v2_remaining_ones_label->setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    _v2_remaining_ones_label->align(LV_ALIGN_BOTTOM_LEFT, 150, 0);
    _v2_remaining_ones_label->removeFlag(LV_OBJ_FLAG_CLICKABLE);
    _v2_remaining_unit_label = std::make_unique<Label>(_v2_remaining_group->get());
    _v2_remaining_unit_label->setText("%");
    _v2_remaining_unit_label->setTextFont(&lv_font_montserrat_36);
    _v2_remaining_unit_label->setTextColor(lv_color_hex(kOwGreen));
    _v2_remaining_unit_label->setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    _v2_remaining_unit_label->alignTo(*_v2_remaining_ones_label, LV_ALIGN_OUT_RIGHT_BOTTOM, 4, 0);
    _v2_remaining_unit_label->removeFlag(LV_OBJ_FLAG_CLICKABLE);
    _v2_left_caption_label = label("5H RESET  --", &lv_font_montserrat_16, 83, 116, 300, kOwSoftText);
    _v2_today_caption_label = label("TODAY", &lv_font_montserrat_16, 43, 158, 58, kOwToday, LV_TEXT_ALIGN_LEFT);
    _v2_today_value_label = label("--%", &lv_font_montserrat_16, 104, 158, 37, kOwToday, LV_TEXT_ALIGN_LEFT);
    _v2_status_label = label("", &lv_font_montserrat_18, 73, 27, 320, kOwBlue);
    _v2_status_label->setHidden(true);
    _v2_meta_left_label = label("WEEK --", &lv_font_montserrat_16, 43, 211, 145, kOwSoftText, LV_TEXT_ALIGN_LEFT);
    _v2_meta_right_label = label("RESET --", &lv_font_montserrat_16, 253, 211, 170, kOwSoftText, LV_TEXT_ALIGN_RIGHT);

    _v2_encoder_feedback_labels[0] = label("", &lv_font_montserrat_20, 101, 126, 39, kOwBlue, LV_TEXT_ALIGN_RIGHT);
    _v2_encoder_feedback_labels[1] = label("", &lv_font_montserrat_20, 326, 126, 39, kOwBlue, LV_TEXT_ALIGN_LEFT);
    for (auto& feedback : _v2_encoder_feedback_labels) {
        feedback->setSize(39, 24);
        feedback->setBgColor(lv_color_hex(kOwBackground));
        feedback->setBgOpa(LV_OPA_COVER);
        feedback->setBorderWidth(0);
        feedback->setPaddingAll(0);
        feedback->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
        feedback->removeFlag(LV_OBJ_FLAG_CLICKABLE);
    }

    _v2_knob = std::make_unique<Container>(_panel->get());
    _v2_knob->setSize(codex::deck::kKnob.width, codex::deck::kKnob.height);
    _v2_knob->align(LV_ALIGN_TOP_LEFT, codex::deck::kKnob.x, codex::deck::kKnob.y);
    // Opaque rectangular backing prevents press feedback repainting the quota scene.
    _v2_knob->setRadius(0); _v2_knob->setPaddingAll(0);
    _v2_knob->setBgColor(lv_color_hex(kOwBackground)); _v2_knob->setBgOpa(255);
    _v2_knob->setBorderWidth(0);
    _v2_knob->removeFlag(LV_OBJ_FLAG_SCROLLABLE); _v2_knob->removeFlag(LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(_v2_knob->get(), &CodexView::drawEncoderButtonsEvent, LV_EVENT_DRAW_MAIN_END, this);

    for (size_t index = 0; index < _v2_agent_hits.size(); ++index) {
        const auto bounds = codex::deck::agentRect(static_cast<int>(index));
        auto hit = std::make_unique<Container>(_panel->get());
        hit->setSize(bounds.width, bounds.height);
        hit->align(LV_ALIGN_TOP_LEFT, bounds.x, bounds.y);
        hit->setRadius(23); hit->setPaddingAll(0);
        hit->setBgOpa(255); hit->setBorderWidth(2);
        hit->removeFlag(LV_OBJ_FLAG_SCROLLABLE); hit->addFlag(LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(hit->get(), &CodexView::nativeAgentTouchEvent, LV_EVENT_ALL, this);
        auto number = std::make_unique<Label>(hit->get());
        const std::string text = std::to_string(index + 1);
        number->setText(text.c_str()); number->setTextFont(&lv_font_montserrat_36);
        number->setTextColor(lv_color_hex(kColorText)); number->align(LV_ALIGN_TOP_MID, 0, 6);
        number->removeFlag(LV_OBJ_FLAG_CLICKABLE);
        auto state = std::make_unique<Label>(hit->get());
        state->setText("离线"); state->setTextFont(&codex_deck_16);
        state->align(LV_ALIGN_BOTTOM_MID, 0, -7); state->removeFlag(LV_OBJ_FLAG_CLICKABLE);
        auto progress = std::make_unique<Container>(hit->get());
        progress->setSize(1, 2); progress->setRadius(1); progress->setBorderWidth(0);
        progress->align(LV_ALIGN_BOTTOM_LEFT, 10, -3); progress->setBgOpa(255); progress->setHidden(true);
        progress->removeFlag(LV_OBJ_FLAG_CLICKABLE); progress->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
        _v2_agent_hits[index] = std::move(hit);
        _v2_agent_numbers[index] = std::move(number);
        _v2_agent_states[index] = std::move(state);
        _v2_agent_progress[index] = std::move(progress);
    }
    updateNativeAgentRail(true);
}

void CodexView::updateOpenWatcherV2Labels()
{
    const int week_pct = static_cast<int>(std::lround(remainingRatio(_state.weekly) * 100.0f));
    const bool week_valid = _state.quotaValid;
    const bool session_valid = week_valid && !_state.stale && _state.session5hLeftPct >= 0 && _state.session5hLeftPct <= 100;
    if (_v2_title_label) {
        const bool link_present = _native_agents_ready || _state.bleConnected;
        const bool codex_live = codexLiveActive();
        const char* title = codex_live
                                ? "Codex Ready"
                                : (link_present ? "Codex Linking" : "Codex Offline");
        const uint32_t color = codex_live
                                   ? kOwBlue
                                   : (link_present ? kOwAmber : kOwStatusIdle);
        setLabelTextIfChanged(*_v2_title_label, title);
        _v2_title_label->setTextColor(lv_color_hex(color));
    }
    if (_v2_remaining_value_label && _v2_remaining_ones_label) {
        const std::string remaining = session_valid ? std::to_string(_state.session5hLeftPct) : "--";
        const size_t ones_index = remaining.size() - 1;
        setLabelTextIfChanged(*_v2_remaining_value_label, remaining.substr(0, ones_index));
        setLabelTextIfChanged(*_v2_remaining_ones_label, remaining.substr(ones_index));
        if (_v2_remaining_unit_label) {
            _v2_remaining_unit_label->alignTo(*_v2_remaining_ones_label, LV_ALIGN_OUT_RIGHT_BOTTOM, 4, 0);
        }
    }
    if (_v2_left_caption_label) {
        const std::string reset = session_valid && !_state.session5hResetLabel.empty()
                                      ? _state.session5hResetLabel : "--";
        setLabelTextIfChanged(*_v2_left_caption_label, "5H RESET  " + reset);
    }
    if (_v2_today_value_label) {
        const bool today_valid = _state.weeklyDayStartLeftPct >= 0;
        const int today_pct = std::clamp(_state.weeklyTodayUsedPctPoints, 0, 100);
        const std::string today = today_valid ? (std::to_string(today_pct) + "%") : "--%";
        setLabelTextIfChanged(*_v2_today_value_label, today);
    }
    if (_v2_status_label) {
        const bool interrupted = _voice_mode == VoiceMode::Interrupted;
        const bool recording = _voice_mode == VoiceMode::Recording;
        const bool processing = _voice_mode == VoiceMode::Processing || _state.processing;
        const bool show_status = interrupted || _state.compactWarning || recording || processing;
        const char* text = interrupted ? "MIC LOST - PRESS A"
                                       : (_state.compactWarning ? "COMPACT SOON"
                                                                : (recording ? "LISTENING" : "PROCESSING"));
        const uint32_t color = interrupted ? kOwToday
                                           : (_state.compactWarning ? kOwAmber
                                                                    : (recording ? kOwGreen : (processing ? kOwAmber : kOwBlue)));
        const char* current = _v2_status_label->getText();
        if (current == nullptr || std::string_view(current) != text) {
            _v2_status_label->setText(text);
            _v2_status_label->setTextColor(lv_color_hex(color));
        }
        setHiddenIfChanged(*_v2_status_label, !show_status);
        setHiddenIfChanged(*_v2_title_label, show_status);
    }
    if (_v2_meta_left_label) {
        const std::string left = week_valid ? std::to_string(week_pct) + "%" : "--";
        setLabelTextIfChanged(*_v2_meta_left_label, "WEEK " + left);
        _v2_meta_left_label->setTextColor(lv_color_hex(week_valid ? quota_semantic_color(week_pct / 100.0f) : kOwSoftText));
    }
    if (_v2_meta_right_label) {
        const std::string reset = week_valid && !_state.weekly.resetInLabel.empty()
                                      ? _state.weekly.resetInLabel : "--";
        setLabelTextIfChanged(*_v2_meta_right_label, "RESET " + reset);
        _v2_meta_right_label->setTextColor(lv_color_hex(week_valid ? kOwBlue : kOwSoftText));
    }
    const bool action_message_visible = _voice_mode == VoiceMode::Idle && _state.messageExpiresAtMs != 0;
    if (_battery_status) setHiddenIfChanged(*_battery_status, action_message_visible);
    if (_bluetooth_status) setHiddenIfChanged(*_bluetooth_status, action_message_visible);
    if (_v2_knob) lv_obj_set_style_opa(_v2_knob->get(), _voice_mode == VoiceMode::Idle ? 255 : 100, 0);
}

void CodexView::updateNativeAgentRail(bool force)
{
    const uint32_t now = GetHAL().millis();
    if (!force && now - _last_native_agent_anim_tick < kNativeAgentAnimationFrameMs) return;
    _last_native_agent_anim_tick = now;
    for (size_t index = 0; index < _v2_agent_hits.size(); ++index) {
        if (!_v2_agent_hits[index]) continue;
        const auto& agent = _native_agents[index];
        const bool assigned = _native_agents_ready && agent.assigned;
        const bool working = assigned && agent.color == kNativeWorkingColor;
        const bool preview = _native_agent_touch_tracking && _native_agent_touch_candidate == static_cast<int>(index);
        // Light assignment affects presentation only. Actions still use transport readiness.
        uint32_t color = assigned && agent.color ? agent.color : 0x526578;
        const char* state = _native_agents_ready ? "未知" : "离线";
        if (assigned) {
            switch (agent.color) {
                case 0x304FFE: state = "执行中"; color = 0x668BFF; break;
                case 0x00FF4C: state = "未读"; break;
                case 0xFF6D00: state = "待处理"; break;
                case 0xFF0033: state = "异常"; break;
                default: state = "状态"; break;
            }
        }
        int opacity = assigned ? static_cast<int>(175 + 80 * clamp01(agent.brightness)) : 105;
        if (working || (assigned && (agent.effect == 4 || agent.effect == 6))) {
            const auto period = static_cast<uint32_t>(2800.0f - 1600.0f * clamp01(agent.speed));
            const float pulse = 0.5f + 0.5f * std::sin((now % period) * 6.28318530718f / period);
            opacity = static_cast<int>((140 + 115 * pulse) * (0.45f + 0.55f * clamp01(agent.brightness)));
        }
        if (preview) opacity = 255;
        const float progress = preview ? (_native_agent_touch_committed ? 1.0f :
            clamp01(static_cast<float>(now - _native_agent_touch_started_at) / kNativeAgentHoldMs)) : 0.0f;
        auto& card = *_v2_agent_hits[index];
        if (_native_agent_card_colors[index] != color) {
            card.setBorderColor(lv_color_hex(color));
            _v2_agent_states[index]->setTextColor(lv_color_hex(color));
            _v2_agent_progress[index]->setBgColor(lv_color_hex(color));
            _native_agent_card_colors[index] = color;
        }
        if (_native_agent_card_opacities[index] != opacity) {
            lv_obj_set_style_border_opa(card.get(), std::clamp(opacity, 50, 255), 0);
            _native_agent_card_opacities[index] = opacity;
        }
        setLabelTextIfChanged(*_v2_agent_states[index], state);
        const uint32_t background = blendRgb(kOwBackground, color, preview ? 0.22f : 0.10f);
        if (_native_agent_card_backgrounds[index] != background) {
            card.setBgColor(lv_color_hex(background));
            _native_agent_card_backgrounds[index] = background;
        }
        const int progress_width = std::max(1, static_cast<int>(66 * progress));
        if (preview && _native_agent_progress_widths[index] != progress_width) {
            _v2_agent_progress[index]->setSize(progress_width, 2);
            _native_agent_progress_widths[index] = progress_width;
        }
        setHiddenIfChanged(*_v2_agent_progress[index], !preview);
    }
}

int CodexView::nativeAgentTargetAt(int x, int y) const
{
    return codex::deck::agentAt(x, y, _native_agent_touch_candidate);
}

void CodexView::resetNativeAgentTouch()
{
    const bool was_tracking = _native_agent_touch_tracking;
    _native_agent_touch_tracking = false;
    _native_agent_touch_candidate = -1;
    _native_agent_touch_started_at = 0;
    _native_agent_touch_last_visual_tick = 0;
    _native_agent_touch_committed = false;
    if (was_tracking) {
        updateNativeAgentRail(true);
    }
}

void CodexView::nativeAgentTouchEvent(lv_event_t* event)
{
    auto* view = static_cast<CodexView*>(lv_event_get_user_data(event));
    if (!view) {
        return;
    }

    const lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        view->resetNativeAgentTouch();
        return;
    }
    if (code != LV_EVENT_PRESSED && code != LV_EVENT_PRESSING) {
        return;
    }
    if (view->_voice_mode != VoiceMode::Idle || view->_task_list_active ||
        view->_action_wheel_active || view->_encoder_touch_tracking) {
        return;
    }

    lv_indev_t* indev = GetHAL().lvTouchpad;
    if (!indev) {
        return;
    }
    lv_point_t point = {0, 0};
    lv_indev_get_point(indev, &point);
    const uint32_t now = GetHAL().millis();
    const int candidate = view->nativeAgentTargetAt(point.x, point.y);

    if (code == LV_EVENT_PRESSED) {
        if (candidate < 0) {
            return;
        }
        view->_native_agent_touch_tracking = true;
        view->_native_agent_touch_candidate = candidate;
        view->_native_agent_touch_started_at = now;
        view->_native_agent_touch_last_visual_tick = now;
        view->_native_agent_touch_committed = false;
        GetHAL().vibrate(14, 35);
        view->updateNativeAgentRail(true);
        return;
    }

    if (!view->_native_agent_touch_tracking) {
        return;
    }
    if (view->_native_agent_touch_committed) return;
    if (candidate != view->_native_agent_touch_candidate) {
        view->_native_agent_touch_candidate = candidate;
        view->_native_agent_touch_started_at = now;
        view->_native_agent_touch_last_visual_tick = now;
        view->_native_agent_touch_committed = false;
        if (candidate >= 0) {
            GetHAL().vibrate(12, 30);
        }
        view->updateNativeAgentRail(true);
        return;
    }
    if (candidate < 0) {
        return;
    }

    if (!view->_native_agent_touch_committed &&
        now - view->_native_agent_touch_started_at >= kNativeAgentHoldMs) {
        view->_native_agent_touch_committed = true;
        const size_t index = static_cast<size_t>(candidate);
        // A slot's light state can be temporarily unassigned after reconnect
        // even though the native Codex Micro HID channel is usable. Queue the
        // physical Agent action unconditionally; the transport layer performs
        // the authoritative ready/online check and reports failure to the UI.
        if (index < view->_native_agents.size()) {
            view->_native_agent_slot_request = candidate;
            GetHAL().vibrate(35, 75);
        } else {
            view->setMessage("AGENT INVALID", 1000);
            GetHAL().vibrate(20, 45);
        }
        view->updateNativeAgentRail(true);
        return;
    }

    if (!view->_native_agent_touch_committed &&
        now - view->_native_agent_touch_last_visual_tick >= kNativeAgentPreviewFrameMs) {
        view->_native_agent_touch_last_visual_tick = now;
        view->updateNativeAgentRail(true);
    }
}

void CodexView::drawOpenWatcherV2Event(lv_event_t* event)
{
    auto* view = static_cast<CodexView*>(lv_event_get_user_data(event));
    if (view == nullptr) {
        return;
    }
    lv_event_code_t code = lv_event_get_code(event);
    if (code != LV_EVENT_DRAW_MAIN_BEGIN) {
        return;
    }

    lv_obj_t* obj = lv_event_get_target_obj(event);
    lv_layer_t* layer = lv_event_get_layer(event);
    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);
    view->drawOpenWatcherV2(layer, coords);
}

void CodexView::drawOpenWatcherV2(lv_layer_t* layer, const lv_area_t& coords)
{
    const int width = coords.x2 - coords.x1 + 1;
    const lv_point_t center = {
        static_cast<lv_coord_t>(coords.x1 + width / 2),
        static_cast<lv_coord_t>(coords.y1 + kScreenCenter),
    };

    lv_draw_arc_dsc_t arc;
    lv_draw_arc_dsc_init(&arc);
    arc.center = center;
    arc.rounded = 1;

    const bool session_valid = _state.quotaValid && !_state.stale &&
                               _state.session5hLeftPct >= 0 && _state.session5hLeftPct <= 100;
    const float session = session_valid
                              ? static_cast<float>(_state.session5hLeftPct) / 100.0f
                              : 0.0f;

    arc.radius = 223;
    arc.width = 7;
    arc.color = lv_color_hex(kOwTrack);
    arc.opa = 170;
    arc.start_angle = 180;
    arc.end_angle = 360;
    lv_draw_arc(layer, &arc);

    constexpr int kTopSegments = 180;
    // An arc task covers its whole circle in LVGL even for a 1-degree stroke.
    // Reject untouched strokes before creating tasks when a modal is removed.
    static const auto segment_bounds = [] {
        std::array<lv_area_t, kTopSegments> bounds{};
        for (int i = 0; i < kTopSegments; ++i) {
            const float a = (180 + i) * 0.01745329252f;
            const float b = (181 + i) * 0.01745329252f;
            const float x1 = 223 * std::cos(a), x2 = 223 * std::cos(b);
            const float y1 = 223 * std::sin(a), y2 = 223 * std::sin(b);
            bounds[i] = {233 + static_cast<int>(std::floor(std::min(x1, x2))) - 9,
                         233 + static_cast<int>(std::floor(std::min(y1, y2))) - 9,
                         233 + static_cast<int>(std::ceil(std::max(x1, x2))) + 9,
                         233 + static_cast<int>(std::ceil(std::max(y1, y2))) + 9};
        }
        return bounds;
    }();
    for (int i = 0; i < kTopSegments; ++i) {
        const float start = static_cast<float>(i) / static_cast<float>(kTopSegments);
        const float end = static_cast<float>(i + 1) / static_cast<float>(kTopSegments);
        if (start >= session) {
            break;
        }
        lv_area_t bounds = segment_bounds[i];
        lv_area_move(&bounds, coords.x1, coords.y1);
        const auto& clip = layer->_clip_area;
        if (bounds.x2 < clip.x1 || bounds.x1 > clip.x2 ||
            bounds.y2 < clip.y1 || bounds.y1 > clip.y2) continue;
        const float active_end = std::min(end, session);
        const int start_deg = 180 + static_cast<int>(std::lround(180.0f * start));
        const int end_deg = 180 + static_cast<int>(std::lround(180.0f * active_end));
        if (end_deg <= start_deg) {
            continue;
        }
        arc.color = lv_color_hex(quota_semantic_color((start + active_end) * 0.5f));
        arc.opa = 235;
        arc.start_angle = start_deg;
        arc.end_angle = end_deg;
        lv_draw_arc(layer, &arc);
    }

    lv_draw_rect_dsc_t rect;
    lv_draw_rect_dsc_init(&rect);

    auto draw_line = [&](int x1, int y1, int x2, int y2, uint32_t color, int line_width, lv_opa_t opa) {
        lv_draw_line_dsc_t line;
        lv_draw_line_dsc_init(&line);
        line.p1 = {static_cast<lv_value_precise_t>(coords.x1 + x1), static_cast<lv_value_precise_t>(coords.y1 + y1)};
        line.p2 = {static_cast<lv_value_precise_t>(coords.x1 + x2), static_cast<lv_value_precise_t>(coords.y1 + y2)};
        line.color = lv_color_hex(color);
        line.width = line_width;
        line.opa = opa;
        line.round_start = 1;
        line.round_end = 1;
        lv_draw_line(layer, &line);
    };

    // Weekly remaining quota. Keep a visible track for missing data. The
    // filled portion uses many short overlapping strokes so its semantic
    // red-to-green color transition stays visually smooth on the round AMOLED.
    constexpr int kWeeklyBarLeft = 43;
    constexpr int kWeeklyBarRight = 423;
    constexpr int kWeeklyBarY = 199;
    constexpr int kWeeklyBarWidth = kWeeklyBarRight - kWeeklyBarLeft;
    constexpr int kWeeklyBarSegments = 96;
    draw_line(kWeeklyBarLeft,
              kWeeklyBarY,
              kWeeklyBarRight,
              kWeeklyBarY,
              kOwTrack,
              8,
              220);
    const bool week_valid = _state.quotaValid;
    if (week_valid) {
        const float remaining = remainingRatio(_state.weekly);
        const int filled_width = static_cast<int>(
            std::lround(static_cast<float>(kWeeklyBarWidth) * remaining));
        for (int segment = 0; segment < kWeeklyBarSegments; ++segment) {
            const int segment_x1 = kWeeklyBarLeft +
                                   (kWeeklyBarWidth * segment) / kWeeklyBarSegments;
            if (segment_x1 >= kWeeklyBarLeft + filled_width) {
                break;
            }
            const int segment_x2 = std::min(
                kWeeklyBarLeft + filled_width,
                kWeeklyBarLeft +
                    (kWeeklyBarWidth * (segment + 1)) / kWeeklyBarSegments);
            const float midpoint = static_cast<float>(
                                       segment_x1 + segment_x2 - 2 * kWeeklyBarLeft) /
                                   (2.0f * static_cast<float>(kWeeklyBarWidth));
            draw_line(segment_x1,
                      kWeeklyBarY,
                      segment_x2,
                      kWeeklyBarY,
                      quota_semantic_color(midpoint),
                      8,
                      255);
        }
    }
    draw_line(42, 235, 424, 235, 0x27394D, 1, 200);

    // Chronological left-to-right; all 24 ten-minute buckets are retained.
    for (int i = 0; i < 24; ++i) {
        lv_area_t area = {coords.x1 + 139 + i * 12, coords.y1 + 242,
                          coords.x1 + 148 + i * 12, coords.y1 + 252};
        rect.radius = 2; rect.bg_color = lv_color_hex(activity_heat_color(_state.activityBuckets[i]));
        rect.bg_opa = 255; rect.border_width = 0;
        lv_draw_rect(layer, &rect, &area);
    }
    lv_draw_label_dsc_t caption;
    lv_draw_label_dsc_init(&caption);
    caption.text = "RECENT 8H"; caption.font = &lv_font_montserrat_14;
    caption.color = lv_color_hex(kOwSoftText);
    const lv_area_t caption_area = {coords.x1 + 42, coords.y1 + 242, coords.x1 + 135, coords.y1 + 257};
    lv_draw_label(layer, &caption, &caption_area);

}

void CodexView::initVoiceWaveform()
{
    _voice_waveform = std::make_unique<Container>(_panel->get());
    _voice_waveform->setSize(118, 42);
    _voice_waveform->align(LV_ALIGN_TOP_MID, 0, 281);
    _voice_waveform->setBgOpa(LV_OPA_TRANSP);
    _voice_waveform->setBorderWidth(0);
    _voice_waveform->setPaddingAll(0);
    _voice_waveform->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
    _voice_waveform->setHidden(true);

    constexpr int kBarCount = 9;
    constexpr int kBarWidth = 8;
    constexpr int kGap = 12;
    constexpr int kStartX = 7;
    for (int i = 0; i < kBarCount; ++i) {
        auto bar = std::make_unique<Container>(_voice_waveform->get());
        bar->setSize(kBarWidth, 10);
        bar->setRadius(5);
        bar->setBorderWidth(0);
        bar->setBgColor(i % 2 == 0 ? lv_color_hex(kOwBlue) : lv_color_hex(0x78DFF5));
        bar->setBgOpa(220);
        bar->setShadowWidth(0);
        bar->align(LV_ALIGN_TOP_LEFT, kStartX + i * kGap, 16);
        bar->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
        _voice_bars[i] = std::move(bar);
    }
}

void CodexView::updateVoiceWaveform()
{
    if (_voice_mode != VoiceMode::Recording ||
        !_voice_waveform || lv_obj_has_flag(_voice_waveform->get(), LV_OBJ_FLAG_HIDDEN)) {
        return;
    }

    constexpr int kMaxHeight = 36;
    constexpr int kMinHeight = 8;
    constexpr int kCenterY = 21;
    const uint32_t now = GetHAL().millis();
    for (size_t i = 0; i < _voice_bars.size(); ++i) {
        if (!_voice_bars[i]) {
            continue;
        }
        const float phase = static_cast<float>((now / 65 + i * 3) % 18) / 18.0f;
        const float wave = 0.5f + 0.5f * std::sin((phase * 6.283185f) + static_cast<float>(i) * 0.78f);
        const int height = kMinHeight + static_cast<int>(std::lround((kMaxHeight - kMinHeight) * wave));
        const int opacity = 150 + static_cast<int>(wave * 90.0f);
        if (_voice_bar_heights[i] == height && _voice_bar_opacities[i] == opacity) {
            continue;
        }
        _voice_bar_heights[i] = height;
        _voice_bar_opacities[i] = opacity;
        _voice_bars[i]->setSize(8, height);
        _voice_bars[i]->align(LV_ALIGN_TOP_LEFT, 7 + static_cast<int>(i) * 12, kCenterY - height / 2);
        _voice_bars[i]->setBgOpa(opacity);
    }
}

void CodexView::updateMotionInput()
{
    const uint32_t now = GetHAL().millis();
    if (now - _last_imu_update_tick < 50) {
        return;
    }
    _last_imu_update_tick = now;

    GetHAL().updateImuData();
    const auto& imu = GetHAL().getImuData();

    const float gyroMotion = std::sqrt(imu.gyroX * imu.gyroX + imu.gyroY * imu.gyroY + imu.gyroZ * imu.gyroZ);
    if (gyroMotion > codex_config::kShakeThreshold) {
        _shake_trigger_count = std::min<uint8_t>(_shake_trigger_count + 1, 3);
    } else if (gyroMotion < codex_config::kShakeThreshold * 0.65f) {
        _shake_trigger_count = 0;
    }

    if (_shake_trigger_count >= 2 && now - _last_shake_tick > codex_config::kShakeCooldownMs) {
        _last_shake_tick = now;
        _shake_trigger_count = 0;
        _clear_input_requested = true;
        GetHAL().vibrate(55, 140);
    }
}

void CodexView::setMessage(const char* message, uint32_t ttlMs)
{
    _state.message              = message;
    _state.messageExpiresAtMs   = GetHAL().millis() + ttlMs;
    updateOpenWatcherV2Labels();
    const bool voice_visible = _voice_mode != VoiceMode::Idle;
    if (_message_label) {
        _message_label->setHidden(voice_visible);
        _message_label->setText(_state.message.c_str());
    }
}

void CodexView::setMessageText(const std::string& message)
{
    if (message.empty() || message == "Ready") {
        _state.message.clear();
        _state.messageExpiresAtMs = 0;
        updateOpenWatcherV2Labels();
        if (_message_label) {
            _message_label->setText("");
            _message_label->setHidden(true);
        }
        return;
    }

    _state.message = message;
    _state.messageExpiresAtMs = GetHAL().millis() + 2200;
    updateOpenWatcherV2Labels();
    if (_message_label) {
        _message_label->setText(_state.message.c_str());
        _message_label->setHidden(_voice_mode != VoiceMode::Idle);
    }
}

void CodexView::initStatusIndicators()
{
    auto make_icon = [&](const codex::deck::Rect& bounds) {
        auto icon = std::make_unique<Container>(_panel->get());
        icon->setSize(bounds.width, bounds.height);
        icon->align(LV_ALIGN_TOP_LEFT, bounds.x, bounds.y);
        icon->setBgColor(lv_color_hex(kOwBackground));
        icon->setBgOpa(LV_OPA_COVER);
        icon->setRadius(0); icon->setBorderWidth(0); icon->setPaddingAll(0);
        icon->removeFlag(LV_OBJ_FLAG_CLICKABLE); icon->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(icon->get(), &CodexView::drawStatusIndicatorEvent, LV_EVENT_DRAW_MAIN, this);
        return icon;
    };
    _battery_status = make_icon(codex::deck::kBatteryStatus);
    _bluetooth_status = make_icon(codex::deck::kBluetoothStatus);
    _battery_value_label = std::make_unique<Label>(_battery_status->get());
    _battery_value_label->setText("--");
    _battery_value_label->setTextFont(&lv_font_montserrat_14);
    _battery_value_label->setTextColor(lv_color_hex(kColorText));
    _battery_value_label->setWidth(38);
    _battery_value_label->setTextAlign(LV_TEXT_ALIGN_CENTER);
    _battery_value_label->align(LV_ALIGN_CENTER, -2, 0);
    _battery_value_label->removeFlag(LV_OBJ_FLAG_CLICKABLE);
    updateStatusIndicators();
}

void CodexView::updateStatusIndicators()
{
    const uint32_t now = GetHAL().millis();
    // Read the HAL's cached percentage, not the PMIC/I2C path. No redraw for
    // unchanged readings, including while the direction panel is open.
    if (_battery_status && (_battery_percent == -2 || now - _last_battery_update_tick >= 1000)) {
        _last_battery_update_tick = now;
        const int reading = GetHAL().getBatteryLevel();
        const int percent = reading <= 100 ? reading : -1;
        if (percent != _battery_percent) {
            _battery_percent = percent;
            const std::string text = percent >= 0 ? std::to_string(percent) : "--";
            if (_battery_value_label) setLabelTextIfChanged(*_battery_value_label, text);
            lv_obj_invalidate(_battery_status->get());
        }
    }
    if (_bluetooth_status && _displayed_ble_connected != static_cast<int>(_state.bleConnected)) {
        _displayed_ble_connected = _state.bleConnected;
        lv_obj_invalidate(_bluetooth_status->get());
    }
}

void CodexView::drawStatusIndicatorEvent(lv_event_t* event)
{
    auto* view = static_cast<CodexView*>(lv_event_get_user_data(event));
    if (!view) return;
    auto* object = lv_event_get_target_obj(event);
    lv_area_t coords;
    lv_obj_get_coords(object, &coords);
    view->drawStatusIndicator(object, lv_event_get_layer(event), coords);
}

void CodexView::drawStatusIndicator(lv_obj_t* object, lv_layer_t* layer, const lv_area_t& coords)
{
    if (object == _battery_status->get()) {
        const uint32_t color = _battery_percent < 0 ? kOwSoftText : (_battery_percent >= 30 ? kOwGreen : kOwAlert);
        lv_draw_rect_dsc_t rect;
        lv_draw_rect_dsc_init(&rect);
        rect.radius = 4; rect.bg_opa = LV_OPA_TRANSP;
        rect.border_width = 2; rect.border_opa = LV_OPA_COVER;
        rect.border_color = lv_color_hex(color);
        const lv_area_t body = {coords.x1, coords.y1 + 2, coords.x1 + 41, coords.y1 + 19};
        lv_draw_rect(layer, &rect, &body);
        rect.radius = 2; rect.border_width = 0;
        rect.bg_color = lv_color_hex(color); rect.bg_opa = LV_OPA_COVER;
        const lv_area_t tip = {coords.x1 + 43, coords.y1 + 7, coords.x1 + 45, coords.y1 + 14};
        lv_draw_rect(layer, &rect, &tip);
        return;
    }
    const uint32_t color = _state.bleConnected ? kOwBlue : kOwAlert;
    lv_draw_line_dsc_t line;
    lv_draw_line_dsc_init(&line);
    line.width = 2; line.round_start = 1; line.round_end = 1;
    line.color = lv_color_hex(color);
    auto stroke = [&](int x1, int y1, int x2, int y2) {
        line.p1 = {coords.x1 + x1, coords.y1 + y1};
        line.p2 = {coords.x1 + x2, coords.y1 + y2};
        lv_draw_line(layer, &line);
    };
    stroke(10, 1, 10, 20);
    stroke(10, 1, 17, 7); stroke(17, 7, 4, 17);
    stroke(4, 5, 17, 14); stroke(17, 14, 10, 20);
    if (!_state.bleConnected) {
        line.color = lv_color_hex(kOwBackground); line.width = 5;
        stroke(2, 1, 19, 20);
        line.color = lv_color_hex(color); line.width = 2;
        stroke(2, 1, 19, 20);
    }
}

float CodexView::remainingRatio(const QuotaSlot& slot) const
{
    if (slot.limit <= 0.0f) {
        return 0.0f;
    }
    return clamp01((slot.limit - slot.used) / slot.limit);
}
