/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <smooth_lvgl.hpp>
#include <smooth_ui_toolkit.hpp>
#include <uitk/short_namespace.hpp>
#include <apps/app_codex/codex_quota_client.h>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace view {

class CodexView {
public:
    enum class ThemeMode : uint8_t {
        CodexMicroDashboard,
        OpenWatcherV2,
    };

    enum class VoiceMode : uint8_t {
        Idle,
        Recording,
        Processing,
        Interrupted,
    };

    struct TaskItem {
        std::string id;
        std::string title;
    };

    struct NativeAgentState {
        bool assigned = false;
        uint32_t color = 0;
        float brightness = 0.0f;
        uint8_t effect = 0;
        float speed = 0.0f;
    };

    struct QuotaSlot {
        float used  = 0.0f;
        float limit = 1.0f;
        std::string resetInLabel;
    };

    struct State {
        QuotaSlot weekly;
        int session5hLeftPct = -1;
        std::string session5hResetLabel = "--";
        int weeklyDayStartLeftPct = -1;
        int weeklySegmentStartLeftPct = -1;
        int weeklyTodayUsedPctPoints = 0;
        int weeklyResetCount = 0;
        std::string weeklyTrackingPeriodStart;
        bool bleConnected  = false;
        bool wifiConnected = false;
        bool processing    = false;
        std::string message;
        std::string source;
        std::string updatedAt;
        bool stale = false;
        bool quotaValid = false;
        std::string hostName;
        std::string sessionTitle = "Codex Ready";
        std::string contextLabel = "-- / --";
        int contextPressurePct = 0;
        int compactThresholdPct = -1;
        bool compactWarning = false;
        std::string totalTokensLabel = "--";
        std::string modelLabel = "--";
        std::string reasoningLabel = "--";
        std::string activityLabel = "--";
        bool activityLive = false;
        bool unreadStateValid = false;
        int unreadTaskCount = 0;
        std::array<float, 24> activityBuckets = {};
        uint32_t messageExpiresAtMs = 0;
    };

    void init(lv_obj_t* parent, ThemeMode themeMode = ThemeMode::CodexMicroDashboard);
    void update();
    void applyQuota(int weeklyLeftPct,
                    const std::string& weeklyResetLabel,
                    bool valid,
                    bool wifiConnected,
                    bool processing,
                    const std::string& message);
    void applySnapshot(const codex::QuotaSnapshot& snapshot);
    void applyBleState(bool connected, const std::string& hostMessage, bool hostMessageChanged);
    void setUnreadTaskCount(int count);
    void setUnreadTasks(const std::vector<TaskItem>& tasks);
    void setNativeAgentStates(bool ready,
                              bool communicating,
                              const std::array<NativeAgentState, 6>& agents);
    void setBatteryState(int percent, bool charging);
    void setPhysicalControlState(bool leftPressed, bool rightPressed);
    void showTaskList();
    std::string consumeTaskOpenRequest();
    int consumeNativeAgentSlotRequest();
    int consumeNativeActionRequest();
    void showActionMessage(const char* message);
    void setVoiceActive(bool active);
    void setVoiceMode(VoiceMode mode);
    bool consumeClearInputRequest();
    int consumeReasoningDeltaRequest();
    bool consumeNativeRadialRequest(float& angle, float& distance);
    uint32_t frameIntervalMs() const;
    bool codexLiveActive() const;
    bool hasExecutingNativeAgent() const;

private:
    std::unique_ptr<uitk::lvgl_cpp::Container> _panel;
    ThemeMode _theme_mode = ThemeMode::CodexMicroDashboard;
    std::unique_ptr<uitk::lvgl_cpp::Label> _message_label;
    std::unique_ptr<uitk::lvgl_cpp::Container> _voice_waveform;
    std::array<std::unique_ptr<uitk::lvgl_cpp::Container>, 9> _voice_bars;
    std::unique_ptr<uitk::lvgl_cpp::Container> _ble_dot;
    std::unique_ptr<uitk::lvgl_cpp::Container> _wifi_dot;
    std::unique_ptr<uitk::lvgl_cpp::Container> _v2_canvas;
    std::unique_ptr<uitk::lvgl_cpp::Label> _v2_title_label;
    std::unique_ptr<uitk::lvgl_cpp::Label> _v2_left_caption_label;
    std::unique_ptr<uitk::lvgl_cpp::Label> _v2_remaining_value_label;
    std::unique_ptr<uitk::lvgl_cpp::Label> _v2_remaining_unit_label;
    std::unique_ptr<uitk::lvgl_cpp::Label> _v2_today_caption_label;
    std::unique_ptr<uitk::lvgl_cpp::Label> _v2_today_value_label;
    std::unique_ptr<uitk::lvgl_cpp::Label> _v2_status_label;
    std::unique_ptr<uitk::lvgl_cpp::Label> _v2_meta_left_label;
    std::unique_ptr<uitk::lvgl_cpp::Label> _v2_meta_right_label;
    std::unique_ptr<uitk::lvgl_cpp::Container> _v2_sync_indicator;
    std::unique_ptr<uitk::lvgl_cpp::Container> _v2_sync_left_line;
    std::unique_ptr<uitk::lvgl_cpp::Container> _v2_sync_right_line;
    std::unique_ptr<uitk::lvgl_cpp::Container> _v2_sync_dot;
    std::array<std::unique_ptr<uitk::lvgl_cpp::Container>, 4> _v2_agent_hits;
    std::array<std::unique_ptr<uitk::lvgl_cpp::Container>, 4> _v2_agent_dots;
    std::unique_ptr<uitk::lvgl_cpp::Container> _micro_canvas;
    std::unique_ptr<uitk::lvgl_cpp::Label> _micro_health_label;
    std::unique_ptr<uitk::lvgl_cpp::Label> _micro_caption_label;
    std::unique_ptr<uitk::lvgl_cpp::Label> _micro_quota_label;
    std::unique_ptr<uitk::lvgl_cpp::Label> _micro_reset_label;
    std::unique_ptr<uitk::lvgl_cpp::Label> _micro_weekly_label;
    std::unique_ptr<uitk::lvgl_cpp::Label> _micro_transient_label;
    std::array<std::unique_ptr<uitk::lvgl_cpp::Label>, 6> _micro_agent_labels;
    std::unique_ptr<uitk::lvgl_cpp::Container> _action_wheel_canvas;
    std::unique_ptr<uitk::lvgl_cpp::Container> _action_wheel_center;
    std::unique_ptr<uitk::lvgl_cpp::Label> _action_wheel_center_label;
    std::array<std::unique_ptr<uitk::lvgl_cpp::Label>, 4> _action_wheel_labels;
    std::unique_ptr<uitk::lvgl_cpp::Container> _task_list_canvas;
    std::unique_ptr<uitk::lvgl_cpp::Label> _task_list_title;
    std::array<std::unique_ptr<uitk::lvgl_cpp::Container>, 5> _task_row_buttons;
    std::array<std::unique_ptr<uitk::lvgl_cpp::Label>, 5> _task_row_labels;
    std::unique_ptr<uitk::lvgl_cpp::Container> _task_list_back_button;
    std::unique_ptr<uitk::lvgl_cpp::Label> _task_list_back_label;
    std::unique_ptr<uitk::lvgl_cpp::Container> _reasoning_canvas;
    std::unique_ptr<uitk::lvgl_cpp::Container> _reasoning_panel;
    std::unique_ptr<uitk::lvgl_cpp::Label> _reasoning_label;
    std::array<int, 9> _voice_bar_heights = {{-1, -1, -1, -1, -1, -1, -1, -1, -1}};
    std::array<int, 9> _voice_bar_opacities = {{-1, -1, -1, -1, -1, -1, -1, -1, -1}};

    State _state;
    uint32_t _last_quota_update_tick = 0;
    uint32_t _last_imu_update_tick = 0;
    uint32_t _last_shake_tick = 0;
    uint8_t _shake_trigger_count = 0;
    bool _clear_input_requested = false;
    bool _action_touch_tracking = false;
    bool _action_wheel_active = false;
    uint32_t _action_touch_started_at = 0;
    int _action_touch_start_x = 0;
    int _action_touch_start_y = 0;
    bool _native_control_blocked_until_release = false;
    int _action_wheel_selection = -1;
    int _native_control_axis = 0;
    bool _native_radial_pending = false;
    float _native_radial_angle = 0.75f;
    float _native_radial_distance = 0.0f;
    uint32_t _last_native_radial_tick = 0;
    std::vector<TaskItem> _unread_tasks;
    std::string _task_open_request;
    std::array<NativeAgentState, 6> _native_agents = {};
    std::array<int, 4> _native_agent_dot_sizes = {{-1, -1, -1, -1}};
    std::array<int, 4> _native_agent_dot_opacities = {{-1, -1, -1, -1}};
    std::array<uint32_t, 4> _native_agent_dot_colors = {};
    bool _native_agents_ready = false;
    bool _native_codex_communicating = false;
    bool _micro_last_live_state = false;
    uint32_t _micro_last_selfheal_ms = 0;
    int _native_agent_slot_request = -1;
    int _native_action_request = -1;
    int _battery_percent = -1;
    bool _battery_charging = false;
    bool _micro_left_pressed = false;
    bool _micro_right_pressed = false;
    bool _micro_touch_tracking = false;
    bool _micro_send_candidate = false;
    bool _micro_swipe_active = false;
    int _micro_touch_start_x = 0;
    int _micro_touch_start_y = 0;
    int _micro_swipe_direction = -1;
    int _micro_completed_agent = -1;
    uint32_t _micro_completed_until_ms = 0;
    bool _native_agents_observed = false;
    uint32_t _last_native_agent_anim_tick = 0;
    bool _native_agent_touch_tracking = false;
    int _native_agent_touch_candidate = -1;
    uint32_t _native_agent_touch_started_at = 0;
    uint32_t _native_agent_touch_last_visual_tick = 0;
    bool _native_agent_touch_committed = false;
    bool _task_list_active = false;
    bool _reasoning_touch_tracking = false;
    int _reasoning_touch_start_x = 0;
    int _reasoning_swipe_direction = 0;
    int _reasoning_swipe_step = 0;
    int _reasoning_delta_request = 0;
    VoiceMode _voice_mode = VoiceMode::Idle;
    void initOpenWatcherV2();
    void updateOpenWatcherV2Labels();
    void updateNativeAgentRail(bool force = false);
    int nativeAgentTargetAt(int x, int y) const;
    void resetNativeAgentTouch();
    static void nativeAgentTouchEvent(lv_event_t* event);
    static void drawOpenWatcherV2Event(lv_event_t* event);
    void drawOpenWatcherV2(lv_layer_t* layer, const lv_area_t& coords);
    void initCodexMicroDashboard();
    void updateCodexMicroDashboardLabels();
    void updateCodexMicroDashboardTouch();
    int codexMicroAgentAt(int x, int y) const;
    static void drawCodexMicroDashboardEvent(lv_event_t* event);
    void drawCodexMicroDashboard(lv_layer_t* layer, const lv_area_t& coords);
    void initActionWheel();
    void updateActionWheelGesture();
    void setActionWheelVisible(bool visible);
    void updateActionWheelSelection(int selection);
    void queueNativeRadial(float angle, float distance);
    void resetNativeTouchControl(bool queueNeutral);
    static void drawActionWheelEvent(lv_event_t* event);
    void drawActionWheel(lv_layer_t* layer, const lv_area_t& coords);
    void initTaskList();
    void setTaskListVisible(bool visible);
    void updateTaskListRows();
    void initReasoningControl();
    void updateReasoningGesture();
    void setReasoningControlVisible(bool visible);
    void updateReasoningControlLabel();
    static void drawReasoningControlEvent(lv_event_t* event);
    void drawReasoningControl(lv_layer_t* layer, const lv_area_t& coords);
    void initVoiceWaveform();
    void updateVoiceWaveform();
    void updateMotionInput();
    void setMessage(const char* message, uint32_t ttlMs);
    void setMessageText(const std::string& message);
    void updateConnectionDots();
    float remainingRatio(const QuotaSlot& slot) const;
};

}  // namespace view
