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

    void init(lv_obj_t* parent);
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
    std::unique_ptr<uitk::lvgl_cpp::Label> _message_label;
    std::unique_ptr<uitk::lvgl_cpp::Container> _voice_waveform;
    std::array<std::unique_ptr<uitk::lvgl_cpp::Container>, 9> _voice_bars;
    std::unique_ptr<uitk::lvgl_cpp::Container> _battery_status;
    std::unique_ptr<uitk::lvgl_cpp::Label> _battery_value_label;
    std::unique_ptr<uitk::lvgl_cpp::Container> _bluetooth_status;
    int _battery_percent = -2;
    int _displayed_ble_connected = -1;
    uint32_t _last_battery_update_tick = 0;
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
    std::array<std::unique_ptr<uitk::lvgl_cpp::Container>, 4> _v2_agent_hits;
    std::array<std::unique_ptr<uitk::lvgl_cpp::Label>, 4> _v2_agent_numbers;
    std::array<std::unique_ptr<uitk::lvgl_cpp::Label>, 4> _v2_agent_states;
    std::array<std::unique_ptr<uitk::lvgl_cpp::Container>, 4> _v2_agent_progress;
    std::unique_ptr<uitk::lvgl_cpp::Container> _v2_knob;
    std::unique_ptr<uitk::lvgl_cpp::Container> _action_wheel_canvas;
    std::unique_ptr<uitk::lvgl_cpp::Container> _radial_detail_canvas;
    std::unique_ptr<uitk::lvgl_cpp::Container> _task_list_canvas;
    std::unique_ptr<uitk::lvgl_cpp::Label> _task_list_title;
    std::array<std::unique_ptr<uitk::lvgl_cpp::Container>, 5> _task_row_buttons;
    std::array<std::unique_ptr<uitk::lvgl_cpp::Label>, 5> _task_row_labels;
    std::unique_ptr<uitk::lvgl_cpp::Container> _task_list_back_button;
    std::unique_ptr<uitk::lvgl_cpp::Label> _task_list_back_label;
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
    int _radial_detail_touch_target = -1;
    bool _native_radial_pending = false;
    float _native_radial_angle = 0.75f;
    float _native_radial_distance = 0.0f;
    std::vector<TaskItem> _unread_tasks;
    std::string _task_open_request;
    std::array<NativeAgentState, 6> _native_agents = {};
    std::array<int, 4> _native_agent_card_opacities = {{-1, -1, -1, -1}};
    std::array<uint32_t, 4> _native_agent_card_colors = {};
    std::array<uint32_t, 4> _native_agent_card_backgrounds = {};
    std::array<int, 4> _native_agent_progress_widths = {{-1, -1, -1, -1}};
    bool _native_agents_ready = false;
    bool _native_codex_communicating = false;
    int _native_agent_slot_request = -1;
    int _native_action_request = -1;
    uint32_t _last_native_agent_anim_tick = 0;
    bool _native_agent_touch_tracking = false;
    int _native_agent_touch_candidate = -1;
    uint32_t _native_agent_touch_started_at = 0;
    uint32_t _native_agent_touch_last_visual_tick = 0;
    bool _native_agent_touch_committed = false;
    bool _task_list_active = false;
    bool _encoder_touch_tracking = false;
    bool _encoder_touch_down = false;
    bool _encoder_touch_cancelled = false;
    int _encoder_pressed_target = -1;
    int _encoder_touch_start_x = 0;
    int _encoder_touch_start_y = 0;
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
    void initActionWheel();
    void updateActionWheelGesture();
    void setActionWheelVisible(bool visible);
    void updateActionWheelSelection(int selection);
    void queueNativeRadial(float angle, float distance);
    void resetNativeTouchControl(bool queueNeutral);
    static void drawActionWheelEvent(lv_event_t* event);
    static void radialDetailTouchEvent(lv_event_t* event);
    void drawActionWheel(lv_layer_t* layer, const lv_area_t& coords);
    void initTaskList();
    void setTaskListVisible(bool visible);
    void updateTaskListRows();
    void updateEncoderTouchControl();
    static void drawEncoderButtonsEvent(lv_event_t* event);
    void drawEncoderButtons(lv_layer_t* layer, const lv_area_t& coords);
    void initVoiceWaveform();
    void updateVoiceWaveform();
    void updateMotionInput();
    void setMessage(const char* message, uint32_t ttlMs);
    void setMessageText(const std::string& message);
    void initStatusIndicators();
    void updateStatusIndicators();
    static void drawStatusIndicatorEvent(lv_event_t* event);
    void drawStatusIndicator(lv_obj_t* object, lv_layer_t* layer, const lv_area_t& coords);
    float remainingRatio(const QuotaSlot& slot) const;
};

}  // namespace view
