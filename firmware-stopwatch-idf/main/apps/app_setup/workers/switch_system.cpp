/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "workers.h"
#include <mooncake_log.h>
#include <assets/assets.h>
#include <esp_ota_ops.h>
#include <esp_app_format.h>
#include <esp_partition.h>
#include <cstdio>

using namespace uitk;
using namespace uitk::lvgl_cpp;
using namespace setup_workers;

static const std::string_view _tag = "Setup-SwitchSys";

/* -------------------------------------------------------------------------- */
/*                            双固件切换（写 otadata）                          */
/* -------------------------------------------------------------------------- */
/*
 * 切到小智：把 ota_0 设为启动槽。ESP-IDF 会重写 otadata 的 ota_seq。
 * 切回本固件：在小智侧擦除整个 otadata，引导器即回落到 factory（官方行为）。
 *
 * 目标槽必须能通过镜像校验，否则宁可不切 —— 避免切进黑屏。
 */
static esp_err_t trySelectOta0Slot()
{
    const esp_partition_t* target =
        esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, nullptr);
    if (target == nullptr) {
        return ESP_ERR_NOT_FOUND;
    }

    // 校验 ota_0 里确实存在一个可启动的固件镜像
    esp_app_desc_t desc = {};
    esp_err_t err       = esp_ota_get_partition_description(target, &desc);
    if (err != ESP_OK) {
        return err;
    }

    mclog::tagInfo(_tag, "ota_0 image: {} v{}", desc.project_name, desc.version);

    // 已经是启动槽就无需重复写
    const esp_partition_t* running = esp_ota_get_running_partition();
    if (running != nullptr && running->address == target->address) {
        return ESP_ERR_INVALID_STATE;
    }

    return esp_ota_set_boot_partition(target);
}

/* -------------------------------------------------------------------------- */
/*                                  确认视图                                   */
/* -------------------------------------------------------------------------- */
class SwitchSystemWorker::SwitchSystemView {
public:
    SwitchSystemView()
    {
        _panel = std::make_unique<Container>(lv_screen_active());
        _panel->align(LV_ALIGN_CENTER, 0, 0);
        _panel->setSize(466, 466);
        _panel->setRadius(0);
        _panel->setBorderWidth(0);
        _panel->setPaddingAll(0);
        _panel->setBgColor(lv_color_hex(0x111318));
        _panel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

        auto line1 = std::make_unique<Label>(_panel->get());
        line1->setTextColor(lv_color_hex(0xF2F3F5));
        line1->setTextFont(&MontserratSemiBold26);
        line1->setText("Switch to");
        line1->align(LV_ALIGN_CENTER, 0, -126);

        auto line2 = std::make_unique<Label>(_panel->get());
        line2->setTextColor(lv_color_hex(0x4292F5));
        line2->setTextFont(&MontserratSemiBold26);
        line2->setText("Xiaozhi AI");
        line2->align(LV_ALIGN_CENTER, 0, -86);

        auto note1 = std::make_unique<Label>(_panel->get());
        note1->setTextColor(lv_color_hex(0x8A9099));
        note1->setTextFont(&lv_font_montserrat_14);
        note1->setText("The device will restart.");
        note1->align(LV_ALIGN_CENTER, 0, -10);

        auto note2 = std::make_unique<Label>(_panel->get());
        note2->setTextColor(lv_color_hex(0x8A9099));
        note2->setTextFont(&lv_font_montserrat_14);
        note2->setText("Codex dashboard and BLE bridge");
        note2->align(LV_ALIGN_CENTER, 0, 18);

        auto note3 = std::make_unique<Label>(_panel->get());
        note3->setTextColor(lv_color_hex(0x8A9099));
        note3->setTextFont(&lv_font_montserrat_14);
        note3->setText("are unavailable in that system.");
        note3->align(LV_ALIGN_CENTER, 0, 44);

        auto hint = std::make_unique<Label>(_panel->get());
        hint->setTextColor(lv_color_hex(0xF2F3F5));
        hint->setTextFont(&lv_font_montserrat_14);
        hint->setText("A  Switch          B  Cancel");
        hint->align(LV_ALIGN_CENTER, 0, 126);

        _status = std::make_unique<Label>(_panel->get());
        _status->setTextColor(lv_color_hex(0xFF6B6B));
        _status->setTextFont(&lv_font_montserrat_14);
        _status->setText(" ");
        _status->align(LV_ALIGN_CENTER, 0, 172);
    }

    void setStatus(const char* text, uint32_t color)
    {
        if (_status) {
            _status->setTextColor(lv_color_hex(color));
            _status->setText(text);
        }
    }

private:
    std::unique_ptr<Container> _panel;
    std::unique_ptr<Label> _status;
};

/* -------------------------------------------------------------------------- */
/*                                 Worker 实现                                 */
/* -------------------------------------------------------------------------- */
SwitchSystemWorker::SwitchSystemWorker()
{
    mclog::tagInfo(_tag, "start switch system worker");

    _view = std::make_unique<SwitchSystemView>();

    // 打开时就先探一次，把“没装小智固件”这种情况立刻告诉用户
    const esp_partition_t* target =
        esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, nullptr);
    esp_app_desc_t desc = {};
    if (target == nullptr) {
        _view->setStatus("ota_0 partition not found", 0xFF6B6B);
        _slot_ready = false;
    } else if (esp_ota_get_partition_description(target, &desc) != ESP_OK) {
        _view->setStatus("Xiaozhi firmware not installed", 0xFF6B6B);
        _slot_ready = false;
    } else {
        char buffer[128] = {};
        std::snprintf(buffer, sizeof(buffer), "slot ready: %s v%s", desc.project_name, desc.version);
        _view->setStatus(buffer, 0x30D158);
        _slot_ready = true;
    }
}

void SwitchSystemWorker::update()
{
    GetHAL().updateButtonStates();

    const uint32_t now = GetHAL().millis();

    // 切换中：让提示停留一小会儿再重启，避免用户以为设备卡死
    if (_state == State::Switching) {
        if (now - _switch_tick >= 700) {
            mclog::tagInfo(_tag, "restarting into ota_0");
            GetHAL().reboot();
        }
        return;
    }

    // 失败后：按任意键回到菜单
    if (_state == State::Failed) {
        if (GetHAL().btnA.wasPressed() || GetHAL().btnB.wasPressed()) {
            _is_done = true;
        }
        return;
    }

    if (GetHAL().btnA.wasPressed()) {
        if (!_slot_ready) {
            _state = State::Failed;
            _view->setStatus("Cannot switch: no image in slot", 0xFF6B6B);
            return;
        }

        const esp_err_t err = trySelectOta0Slot();
        if (err == ESP_OK) {
            _state       = State::Switching;
            _switch_tick = now;
            _view->setStatus("Restarting into Xiaozhi...", 0x30D158);
        } else if (err == ESP_ERR_INVALID_STATE) {
            // 已经在 ota_0 上运行，理论上不该走到这里
            _state = State::Failed;
            _view->setStatus("Already booted from that slot", 0xFFB020);
        } else {
            mclog::tagError(_tag, "set boot partition failed: {}", esp_err_to_name(err));
            _state = State::Failed;
            _view->setStatus("Switch failed, see log", 0xFF6B6B);
        }
        return;
    }

    if (GetHAL().btnB.wasPressed()) {
        _is_done = true;
    }
}

SwitchSystemWorker::~SwitchSystemWorker()
{
}
