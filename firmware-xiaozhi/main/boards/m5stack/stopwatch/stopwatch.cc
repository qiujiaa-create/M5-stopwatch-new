/*
 * M5Stack StopWatch (Tangaro) board support for xiaozhi-esp32 v2.2.6
 *
 * 目标：让手表以“双固件 A/B 槽”的方式运行小智，与原有 StopWatch/Codex 固件互不干扰。
 *
 * 本文件参考 main/boards/waveshare/esp32-s3-touch-amoled-1.75（同样是 CO5300 QSPI + 466x466 圆屏），
 * 但硬件差异按本机已验证的 StopWatch 固件 HAL 取值：
 *   - 显示 QSPI 引脚不同（SCLK40 / D0-D3 41,42,46,45 / CS39）
 *   - 无 TCA9554，改由 M5IOE1 (PY32) 承担屏幕 RST/使能、喇叭 PA、触摸 RST
 *   - PMIC 为 M5PM1（非 AXP2101）
 *   - 按键 A=GPIO2 / B=GPIO1
 *
 * 一期范围（按用户确认）：跑通小智本体（语音对话）。日程/待办/笔记不做。
 * 触摸（CST820）一期未接入，UI 通过按键操作。
 */

#include "wifi_board.h"
#include "display/lcd_display.h"
#include "esp_lcd_co5300.h"

#include "codecs/es8311_audio_codec.h"
#include "application.h"
#include "button.h"
#include "mcp_server.h"
#include "config.h"
#include "power_save_timer.h"
#include "settings.h"

#include <esp_log.h>
#include <esp_lcd_panel_vendor.h>
#include <driver/i2c_master.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <M5PM1.h>
#include <M5IOE1.h>

#include <esp_lvgl_port.h>
#include <lvgl.h>

#define TAG "M5StackStopWatch"

/* -------------------------------------------------------------------------- */
/*                          M5IOE1 (PY32) 引脚映射                            */
/* 取自 StopWatch 固件 main/hal/hal_ioe.cpp                                    */
/* -------------------------------------------------------------------------- */
#define PY32_MOTOR_EN_PIN M5IOE1_PIN_9   /* 震动马达使能 (PWM)   */
#define PY32_L3B_EN_PIN   M5IOE1_PIN_8   /* 屏幕使能/升压        */
#define PY32_SPK_PA_PIN   M5IOE1_PIN_10  /* 喇叭 PA 一级门控     */
#define PY32_TP_RST_PIN   M5IOE1_PIN_4   /* 触摸复位             */
#define PY32_OLED_RST_PIN M5IOE1_PIN_5   /* 屏幕复位             */
#define PY32_MUX_CTR_PIN  M5IOE1_PIN_1   /* CH442E MUX 控制      */
#define PY32_AU_EN_PIN    M5IOE1_PIN_3   /* 音频使能             */

/* M5IOE1 在总线上有两个可能地址，StopWatch 固件也是先 0x4F 再退回 0x6F */
#define IOE_ADDR_PRIMARY   0x4F
#define IOE_ADDR_SECONDARY 0x6F

/* M5PM1 充电状态引脚 */
#define PMG_CHG_STAT M5PM1_GPIO_NUM_2
/* M5PM1 充电编程引脚：拉低 = 强制「主动充电编程」。
 * 原厂 StopWatch 固件在 pmic_init() 里就拉低了，小智板卡漏了这一步。 */
#define PMG_CHG_PROG M5PM1_GPIO_NUM_3

/* CO5300 的 QSPI 命令 opcode（与 esp_lcd_panel_io_tx_param 配合写亮度） */
#define LCD_OPCODE_WRITE_CMD (0x02ULL)

/*
 * 屏幕"被关掉"的判定阈值。
 * CO5300 是 AMOLED —— 逐像素发光，亮度寄存器（0x51）写 0 时面板**全黑但仍在工作**，
 * 肉眼和"没点亮"完全分不出来。小智的 MCP 工具 `self.screen.set_brightness` 可以让
 * 语音把亮度写成 0（还会持久化进 NVS），所以按键唤醒时必须显式判这个阈值。
 */
static constexpr uint8_t kMinVisibleBrightness = 20;

/* 开机自愈 / 唤醒兜底用的正常亮度（与 Backlight::RestoreBrightness() 的默认值一致） */
static constexpr uint8_t kDefaultBrightness = 75;

/* 电池电压 -> 百分比；曲线沿用 StopWatch 固件 hal_pmic.cpp 的实测标定（3300mV=0%, 4200mV=100%） */
static uint8_t BatteryMillivoltsToPercent(uint16_t millivolts)
{
    constexpr uint16_t kEmptyMv = 3300;
    constexpr uint16_t kFullMv  = 4200;

    if (millivolts <= kEmptyMv) {
        return 0;
    }
    if (millivolts >= kFullMv) {
        return 100;
    }
    const uint32_t scaled = (uint32_t)(millivolts - kEmptyMv) * 100U / (kFullMv - kEmptyMv);
    return (uint8_t)(scaled > 100U ? 100U : scaled);
}

/*
 * CO5300 初始化序列。
 * 前 4 条来自 waveshare 1.75 板卡（把 CO5300 切到 QSPI 模式，是 esp_lcd + QSPI 路径的必需步骤）；
 * 其余面板参数沿用 1.75（同为 466x466 CO5300，且它同样用 set_gap(6,0) 对齐本机面板偏移）。
 */
static const co5300_lcd_init_cmd_t vendor_specific_init[] = {
    // set display to qspi mode
    {0xFE, (uint8_t[]){0x20}, 1, 0},
    {0x19, (uint8_t[]){0x10}, 1, 0},
    {0x1C, (uint8_t[]){0xA0}, 1, 0},

    {0xFE, (uint8_t[]){0x00}, 1, 0},
    {0xC4, (uint8_t[]){0x80}, 1, 0},
    {0x3A, (uint8_t[]){0x55}, 1, 0},  // pixel format: RGB565
    {0x35, (uint8_t[]){0x00}, 1, 0},
    {0x53, (uint8_t[]){0x20}, 1, 0},
    {0x51, (uint8_t[]){0xFF}, 1, 0},
    {0x63, (uint8_t[]){0xFF}, 1, 0},
    {0x2A, (uint8_t[]){0x00, 0x06, 0x01, 0xD7}, 4, 0},  // column 6..471
    {0x2B, (uint8_t[]){0x00, 0x00, 0x01, 0xD1}, 4, 600}, // row    0..465
    {0x11, NULL, 0, 600},                                // sleep out
    {0x29, NULL, 0, 0},                                  // display on
};

/* -------------------------------------------------------------------------- */
/*                    与本机 StopWatch 固件之间的切换                          */
/* -------------------------------------------------------------------------- */

/*
 * 切回出厂槽（factory = 原有 StopWatch/Codex 固件）：
 *   擦除 otadata 分区即可。ESP-IDF 引导器在 otadata 无效时优先选择 factory 分区
 *   （见 components/app_update/esp_ota_ops.c 的 find_default_boot_partition）。
 *   这是官方机制，不需要改二级引导器，也不会变砖。
 */
static bool SwitchBackToStopWatchSystem()
{
    const esp_partition_t* otadata =
        esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, nullptr);
    if (otadata == nullptr) {
        ESP_LOGE(TAG, "otadata partition not found; cannot switch back");
        return false;
    }

    esp_err_t err = esp_partition_erase_range(otadata, 0, otadata->size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "erase otadata failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGW(TAG, "otadata erased -> bootloader will boot factory (StopWatch firmware)");
    return true;
}

/* -------------------------------------------------------------------------- */
/*                                  显示适配                                   */
/* -------------------------------------------------------------------------- */
class StopWatchLcdDisplay : public SpiLcdDisplay {
public:
    // 圆屏 + 2 像素对齐：避免 LVGL 局部刷新落在奇数边界上产生撕裂
    static void rounder_event_cb(lv_event_t* e)
    {
        lv_area_t* area = (lv_area_t*)lv_event_get_param(e);
        area->x1        = (area->x1 >> 1) << 1;
        area->y1        = (area->y1 >> 1) << 1;
        area->x2        = ((area->x2 >> 1) << 1) + 1;
        area->y2        = ((area->y2 >> 1) << 1) + 1;
    }

    StopWatchLcdDisplay(esp_lcd_panel_io_handle_t io_handle, esp_lcd_panel_handle_t panel_handle, int width, int height,
                        int offset_x, int offset_y, bool mirror_x, bool mirror_y, bool swap_xy)
        : SpiLcdDisplay(io_handle, panel_handle, width, height, offset_x, offset_y, mirror_x, mirror_y, swap_xy)
    {
    }

    virtual void SetupUI() override
    {
        SpiLcdDisplay::SetupUI();

        DisplayLockGuard lock(this);
        // 圆屏：状态栏左右留白，避免被圆角裁掉
        lv_obj_set_style_pad_left(status_bar_, LV_HOR_RES * 0.1, 0);
        lv_obj_set_style_pad_right(status_bar_, LV_HOR_RES * 0.1, 0);
        lv_display_add_event_cb(display_, rounder_event_cb, LV_EVENT_INVALIDATE_AREA, NULL);
    }
};

/* 亮度走 CO5300 的 0x51 命令（本机面板没有独立背光 GPIO） */
class StopWatchBacklight : public Backlight {
public:
    StopWatchBacklight(esp_lcd_panel_io_handle_t panel_io) : Backlight(), panel_io_(panel_io) {}

    /* 关屏前最后一次"非零且可见"的目标亮度，唤醒时用它精确还原 */
    uint8_t LastGoodBrightness() const
    {
        return last_good_brightness_;
    }

    /* 当前的目标亮度（SetBrightness 只改目标值，brightness_ 要等渐变走完才追上） */
    uint8_t TargetBrightness() const
    {
        return target_brightness_;
    }

protected:
    esp_lcd_panel_io_handle_t panel_io_;
    uint8_t last_good_brightness_ = kDefaultBrightness;

    virtual void SetBrightnessImpl(uint8_t brightness) override
    {
        /*
         * 借这里记录"上一个非零目标亮度"。
         * 关键点：只看 target_brightness_（用户/语音请求的目标值），
         * 不能看 brightness（渐变中间值）—— 关屏 50→0 时中间值会一路衰减到 1，
         * 拿那些值当历史的话唤醒后只有亮度 1，等于没亮。
         */
        if (target_brightness_ >= kMinVisibleBrightness) {
            last_good_brightness_ = target_brightness_;
        }

        auto display = Board::GetInstance().GetDisplay();
        if (display == nullptr) {
            return;
        }
        DisplayLockGuard lock(display);
        uint8_t data[1] = {(uint8_t)((255 * brightness) / 100)};
        uint32_t lcd_cmd = 0x51;
        lcd_cmd &= 0xff;
        lcd_cmd <<= 8;
        lcd_cmd |= LCD_OPCODE_WRITE_CMD << 24;
        const esp_err_t err = esp_lcd_panel_io_tx_param(panel_io_, lcd_cmd, &data, sizeof(data));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "set panel brightness failed: %s", esp_err_to_name(err));
        }
    }
};

/* -------------------------------------------------------------------------- */
/*                                  板卡本体                                   */
/* -------------------------------------------------------------------------- */
class M5StackStopWatch : public WifiBoard {
private:
    i2c_master_bus_handle_t i2c_bus_ = nullptr;
    M5PM1* pmic_                     = nullptr;
    M5IOE1* ioe_                     = nullptr;

    Button boot_button_;   // Button A (GPIO2)：单击说话
    Button back_button_;   // Button B (GPIO1)：长按切回 StopWatch 系统

    StopWatchLcdDisplay* display_  = nullptr;
    StopWatchBacklight* backlight_ = nullptr;
    PowerSaveTimer* power_save_timer_ = nullptr;

    /*
     * 屏幕本来是黑的、按键把它点亮 -> 这一次按键只用于"点灯"，
     * 松手时的 OnClick 就不再触发说话，避免摸黑按一下就误对话。
     * 在 OnPressDown 里锁存（那时屏幕还是黑着，能准确判断），OnClick 里消费。
     */
    bool suppress_next_click_ = false;

    /* 电量一阶滤波（加权 7:1，沿用 StopWatch 固件的取值），避免 ADC 抖动导致电量跳变 */
    uint16_t battery_filtered_mv_ = 0;

    /* ---------------- 电源管理 ---------------- */
    void InitializePowerSaveTimer()
    {
        /* 60s 无操作进省电（背光降到 20%）；15 分钟无操作才关机 —— 阈值对齐原 StopWatch 固件。
         * 原来写的是 300s（5 分钟），对一块手表来说太激进。 */
        power_save_timer_ = new PowerSaveTimer(-1, 60, 900);
        power_save_timer_->OnEnterSleepMode([this]() {
            GetDisplay()->SetPowerSaveMode(true);
            GetBacklight()->SetBrightness(20);
        });
        power_save_timer_->OnExitSleepMode([this]() {
            GetDisplay()->SetPowerSaveMode(false);
            GetBacklight()->RestoreBrightness();
        });
        power_save_timer_->OnShutdownRequest([this]() {
            if (pmic_ == nullptr) {
                return;
            }
            /* 接着 USB 外部供电时绝不自动关机：PMIC 一旦关掉输出，SoC 仍被 USB 供着，
             * 会停在「芯片在跑、屏幕全黑」的僵尸态，软复位救不回来。 */
            uint16_t vin_mv = 0;
            if (pmic_->readVin(&vin_mv) == M5PM1_OK && vin_mv > 4000) {
                ESP_LOGW(TAG, "external power present (vin=%u mV); skip auto shutdown", vin_mv);
                power_save_timer_->WakeUp();
                return;
            }
            ESP_LOGW(TAG, "idle shutdown (vin=%u mV)", vin_mv);
            pmic_->shutdown();
        });

        /* 不在这里无条件 SetEnabled(true)。原固件只在「放电（未接外部电源）」时才启用，
         * 否则插着 USB 也会被闲置关机 —— 这正是之前黑屏的触发条件。
         * 实际开关交给 GetBatteryLevel() 里的充放电判定。 */
    }

    /* ---------------- 屏幕唤醒 ---------------- */
    /*
     * 屏幕是否处于"被关掉"的状态。
     * 注意只能用 backlight_->brightness()（已生效的亮度）：小智的
     * `self.screen.set_brightness` 是让语音直接把亮度写成 0，不经过 PowerSaveTimer，
     * 所以不能靠 in_sleep_mode_ 判断。省电模式的亮度是 20，正好不落进这个阈值。
     */
    bool IsScreenOff() const
    {
        return backlight_ != nullptr && backlight_->brightness() < kMinVisibleBrightness;
    }

    /*
     * 唤醒屏幕。
     *
     * 为什么需要它：小智的 `PowerSaveTimer::WakeUp()` **只在 in_sleep_mode_ == true 时**
     * 才恢复背光；语音关屏走的是 brightness 直写，in_sleep_mode_ 始终为 false，
     * 于是按键触发的 SetPowerSaveLevel() -> WakeUp() 是空操作，屏幕永远黑着
     * （设备其实一直是好的，音频链路不受显示影响，所以语音还"能唤醒它"）。
     */
    void WakeScreen()
    {
        if (power_save_timer_ != nullptr) {
            power_save_timer_->WakeUp(); /* 省电态会顺带恢复背光；非省电态是空操作 */
        }
        if (!IsScreenOff()) {
            return;
        }
        const uint8_t target = backlight_->LastGoodBrightness();
        ESP_LOGI(TAG, "screen was off; waking up with brightness %u", target);
        /* permanent=true：顺手把 NVS 里被语音写成 0 的亮度修好，
         * 否则下次重启 RestoreBrightness() 只会兜底成 10（很暗） */
        backlight_->SetBrightness(target, true);
    }

    /* ---------------- I2C ---------------- */
    void InitializeI2c()
    {
        i2c_master_bus_config_t cfg = {};
        cfg.i2c_port                = I2C_NUM_0;
        cfg.sda_io_num              = AUDIO_CODEC_I2C_SDA_PIN;
        cfg.scl_io_num              = AUDIO_CODEC_I2C_SCL_PIN;
        cfg.clk_source              = I2C_CLK_SRC_DEFAULT;
        cfg.glitch_ignore_cnt       = 7;
        cfg.intr_priority           = 0;
        cfg.trans_queue_depth       = 0;
        cfg.flags.enable_internal_pullup = 1;
        ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &i2c_bus_));
    }

    /* ---------------- 电源芯片 M5PM1 ---------------- */
    void InitializePmic()
    {
        pmic_ = new M5PM1();
        if (pmic_->begin(i2c_bus_) != M5PM1_OK) {
            ESP_LOGE(TAG, "M5PM1 init failed");
            delete pmic_;
            pmic_ = nullptr;
            return;
        }

        pmic_->setI2cSleepTime(0);
        pmic_->btnSetConfig(M5PM1_BTN_TYPE_CLICK, M5PM1_BTN_CLICK_DELAY_1000MS);
        pmic_->wdtSet(0);              /* 关闭 PMIC 看门狗（默认开启） */
        pmic_->ldoSetPowerHold(true);  /* 关机时保持 RTC 供电 */
        pmic_->setChargeEnable(true);

        /*
         * 把 CHG_PROG 拉低，强制启用「主动充电编程」。
         * 原厂 StopWatch 固件 (hal_pmic.cpp 的 Hal::pmic_init) 有这一步，小智板卡漏了。
         * 漏掉的后果：插着 USB 也充不进电 -> 电量一路掉到低电保护 ->
         * PMIC 关掉屏幕升压(L3B) -> 出现「芯片靠 USB 还活着、屏幕全黑」的假死，
         * 软复位救不回来（必须跑一次原固件，或真断电重开）。
         */
        pmic_->gpioSet(PMG_CHG_PROG, M5PM1_GPIO_MODE_OUTPUT, 0, M5PM1_GPIO_PULL_NONE,
                       M5PM1_GPIO_DRIVE_PUSHPULL);

        pmic_->setSingleResetDisable(true);

        /* 充电状态引脚设为输入 */
        pmic_->gpioSetFunc(PMG_CHG_STAT, M5PM1_GPIO_FUNC_GPIO);
        pmic_->gpioSetMode(PMG_CHG_STAT, M5PM1_GPIO_MODE_INPUT);
        pmic_->gpioSetPull(PMG_CHG_STAT, M5PM1_GPIO_PULL_NONE);

        /* 开机报一次电池电压，便于判断"黑屏是不是低电导致的" */
        uint16_t vbat_mv = 0;
        if (pmic_->readVbat(&vbat_mv) == M5PM1_OK) {
            ESP_LOGI(TAG, "battery at boot: %u mV", vbat_mv);
        }

        ESP_LOGI(TAG, "M5PM1 init ok");
    }

    /* ---------------- IO 扩展 M5IOE1 ---------------- */
    /*
     * 这一步是显示与音频的前置条件：屏幕复位、屏幕使能、喇叭 PA 都在 PY32 上。
     * 顺序沿用 StopWatch 固件 hal_ioe.cpp 的初始化序列。
     */
    void InitializeIoe()
    {
        ioe_ = new M5IOE1();
        uint8_t addr = IOE_ADDR_PRIMARY;
        m5ioe1_err_t ret = ioe_->begin(i2c_bus_, IOE_ADDR_PRIMARY, M5IOE1_I2C_FREQ_400K);
        if (ret != M5IOE1_OK) {
            addr = IOE_ADDR_SECONDARY;
            ret  = ioe_->begin(i2c_bus_, IOE_ADDR_SECONDARY, M5IOE1_I2C_FREQ_400K);
        }
        if (ret != M5IOE1_OK) {
            ESP_LOGE(TAG, "M5IOE1 init failed");
            delete ioe_;
            ioe_ = nullptr;
            return;
        }
        ESP_LOGI(TAG, "M5IOE1 init ok, addr=0x%02X", addr);

        ioe_->setI2cSleepTime(0);

        ioe_->pinMode(PY32_MOTOR_EN_PIN, OUTPUT);
        ioe_->pinMode(PY32_L3B_EN_PIN, OUTPUT);
        ioe_->pinMode(PY32_SPK_PA_PIN, OUTPUT);
        ioe_->pinMode(PY32_TP_RST_PIN, OUTPUT);
        ioe_->pinMode(PY32_OLED_RST_PIN, OUTPUT);
        ioe_->pinMode(PY32_MUX_CTR_PIN, OUTPUT);
        ioe_->pinMode(PY32_AU_EN_PIN, OUTPUT);

        /*
         * PMIC 双击关机和跨固件软重启都可能让 CO5300 保留上一轮的状态。
         * 先保持面板复位，再实际断开一次 L3B，避免仅复位 ESP32 时面板仍在旧状态。
         */
        ioe_->digitalWrite(PY32_OLED_RST_PIN, 0);
        ioe_->digitalWrite(PY32_L3B_EN_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
        ioe_->digitalWrite(PY32_L3B_EN_PIN, 1);
        bool l3b_ready = false;
        for (int attempt = 0; attempt < 5; ++attempt) {
            vTaskDelay(pdMS_TO_TICKS(20));
            if (ioe_->digitalRead(PY32_L3B_EN_PIN) == 1) {
                l3b_ready = true;
                break;
            }
            ioe_->digitalWrite(PY32_L3B_EN_PIN, 1);
        }
        if (!l3b_ready) {
            ESP_LOGE(TAG, "display power enable did not go high");
        } else {
            ESP_LOGI(TAG, "display power cycled and L3B enable verified high");
        }
        ioe_->digitalWrite(PY32_SPK_PA_PIN, 1);   /* 喇叭一级门控常开，二级由 GPIO14 控制 */
        ioe_->digitalWrite(PY32_TP_RST_PIN, 1);
        ioe_->digitalWrite(PY32_MUX_CTR_PIN, 0);
        ioe_->digitalWrite(PY32_AU_EN_PIN, 1);

        ioe_->setPwmFrequency(5000);

        /* 等屏幕电源稳定后再释放复位，并让 esp_lcd 发初始化序列 */
        vTaskDelay(pdMS_TO_TICKS(150));
    }

    /* 屏幕硬复位（面板 RST 没有直连 GPIO，走 PY32） */
    void ResetPanelViaIoe()
    {
        if (ioe_ == nullptr) {
            return;
        }
        ioe_->digitalWrite(PY32_OLED_RST_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(150));
        if (ioe_->digitalRead(PY32_OLED_RST_PIN) != 1) {
            ESP_LOGE(TAG, "panel reset release did not go high");
        }
    }

    /* ---------------- QSPI ---------------- */
    void InitializeSpi()
    {
        spi_bus_config_t buscfg = {};
        buscfg.sclk_io_num      = EXAMPLE_PIN_NUM_LCD_PCLK;
        buscfg.data0_io_num     = EXAMPLE_PIN_NUM_LCD_DATA0;
        buscfg.data1_io_num     = EXAMPLE_PIN_NUM_LCD_DATA1;
        buscfg.data2_io_num     = EXAMPLE_PIN_NUM_LCD_DATA2;
        buscfg.data3_io_num     = EXAMPLE_PIN_NUM_LCD_DATA3;
        buscfg.max_transfer_sz  = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        buscfg.flags            = SPICOMMON_BUSFLAG_QUAD;
        ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    /* ---------------- 显示 ---------------- */
    void InitializeDisplay()
    {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel       = nullptr;

        esp_lcd_panel_io_spi_config_t io_config =
            CO5300_PANEL_IO_QSPI_CONFIG(EXAMPLE_PIN_NUM_LCD_CS, nullptr, nullptr);
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &panel_io));

        const co5300_vendor_config_t vendor_config = {
            .init_cmds      = &vendor_specific_init[0],
            .init_cmds_size = sizeof(vendor_specific_init) / sizeof(co5300_lcd_init_cmd_t),
            .flags          = {.use_qspi_interface = 1},
        };

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST;  /* NC：复位由 PY32 代管 */
        panel_config.rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB;
        panel_config.bits_per_pixel = 16;
        panel_config.vendor_config  = (void*)&vendor_config;
        ESP_ERROR_CHECK(esp_lcd_new_panel_co5300(panel_io, &panel_config, &panel));

        esp_lcd_panel_set_gap(panel, DISPLAY_GAP_X, DISPLAY_GAP_Y);

        ResetPanelViaIoe();          /* 先硬复位，再让驱动发初始化命令 */

        esp_lcd_panel_reset(panel);
        esp_lcd_panel_init(panel);
        esp_lcd_panel_invert_color(panel, false);
        esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
        esp_lcd_panel_disp_on_off(panel, true);

        display_ = new StopWatchLcdDisplay(panel_io, panel, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_OFFSET_X,
                                           DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
        backlight_ = new StopWatchBacklight(panel_io);
        backlight_->RestoreBrightness();

        /*
         * 自愈：语音关屏会把 0 持久化进 NVS 的 display/brightness，
         * 重启后 RestoreBrightness() 只会兜底成 10（很暗，看着像没亮）。
         * 这里把过低的持久化值直接修回正常亮度，一次治本。
         * 判据用 TargetBrightness()（请求值）而不是 brightness()（渐变还没跑，此刻还是 0）。
         */
        if (backlight_->TargetBrightness() < kMinVisibleBrightness) {
            ESP_LOGW(TAG, "persisted brightness (%u) is too low; forcing %u",
                     backlight_->TargetBrightness(), kDefaultBrightness);
            backlight_->SetBrightness(kDefaultBrightness, true);
        }
    }

    /* ---------------- 按键 ---------------- */
    void InitializeButtons()
    {
        /*
         * 两个按键都先做「按下即唤醒屏幕」。
         * 用 OnPressDown 而不是 OnClick：一按就亮更跟手，而且和 OnLongPress
         * （Button B 长按切系统）不冲突，不会因为唤醒而误触发业务动作。
         */
        boot_button_.OnPressDown([this]() {
            /* 此刻屏幕还是黑的，判断才准确 —— 先锁存，唤醒后 OnClick 再消费 */
            suppress_next_click_ = IsScreenOff();
            WakeScreen();
        });

        back_button_.OnPressDown([this]() {
            WakeScreen(); /* Button B 没有单击业务，所以"按一下"就是单纯点灯 */
        });

        /* Button A：单击说话（小智默认交互） */
        boot_button_.OnClick([this]() {
            if (suppress_next_click_) {
                /* 屏幕原本是黑的，这一下只用来点灯，不进入说话 */
                suppress_next_click_ = false;
                ESP_LOGI(TAG, "screen wake press consumed; skip chat toggle");
                return;
            }
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            app.ToggleChatState();
        });

        /* Button A：长按进入配网 */
        boot_button_.OnLongPress([this]() {
            suppress_next_click_ = false; /* 长按走自己的路径，别把锁存留到下次 */
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateIdle) {
                EnterWifiConfigMode();
            }
        });

        /*
         * Button B：长按 3 秒切回 StopWatch/Codex 系统。
         * 放在长按而不是单击，是为了避免误触把一个系统切走（切换会重启设备）。
         */
        back_button_.OnLongPress([this]() {
            auto& app = Application::GetInstance();
            app.Schedule([this]() {
                ESP_LOGW(TAG, "user requested switch back to StopWatch system");
                if (SwitchBackToStopWatchSystem()) {
                    vTaskDelay(pdMS_TO_TICKS(200));
                    esp_restart();
                } else {
                    ESP_LOGE(TAG, "switch back failed; staying in xiaozhi");
                }
            });
        });
    }

    /* ---------------- MCP 工具 ---------------- */
    void InitializeTools()
    {
        auto& mcp_server = McpServer::GetInstance();

        mcp_server.AddTool("self.system.reconfigure_wifi",
                           "End this conversation and enter WiFi configuration mode.\n"
                           "**CAUTION** You must ask the user to confirm this action.",
                           PropertyList(), [this](const PropertyList& properties) {
                               EnterWifiConfigMode();
                               return true;
                           });

        /*
         * 允许语音切回手表原生系统（需用户口头确认后由大模型触发）。
         * 设备会重启并进入 StopWatch/Codex 固件。
         */
        mcp_server.AddTool("self.system.switch_to_stopwatch",
                           "Reboot this device and switch back to the native StopWatch (Codex) firmware.\n"
                           "**CAUTION** You must ask the user to confirm this action first, "
                           "because the device will restart and the current conversation ends.",
                           PropertyList(), [this](const PropertyList& properties) {
                               Application::GetInstance().Schedule([]() {
                                   if (SwitchBackToStopWatchSystem()) {
                                       vTaskDelay(pdMS_TO_TICKS(200));
                                       esp_restart();
                                   }
                               });
                               return true;
                           });
    }

public:
    M5StackStopWatch()
        : boot_button_(USER_BUTTON_A_GPIO, false, 3000), back_button_(USER_BUTTON_B_GPIO, false, 3000)
    {
        InitializePowerSaveTimer();
        InitializeI2c();
        InitializePmic();
        InitializeIoe();       /* 必须在显示与音频之前：屏幕复位/使能都靠它 */
        InitializeSpi();
        InitializeDisplay();
        InitializeButtons();
        InitializeTools();
    }

    virtual AudioCodec* GetAudioCodec() override
    {
        /*
         * ES8311 单声道：24k 采样（小智默认档位；本机 ES8311 两条路径都已验证可用）。
         * pa_pin 用 GPIO14 —— PA 的第二级门控；第一级(PY32_SPK_PA)已在 InitializeIoe 里常开。
         */
        static Es8311AudioCodec audio_codec(i2c_bus_, I2C_NUM_0, AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
                                            AUDIO_I2S_GPIO_MCLK, AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS,
                                            AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN, AUDIO_CODEC_PA_PIN,
                                            AUDIO_CODEC_ES8311_ADDR);
        return &audio_codec;
    }

    virtual Display* GetDisplay() override
    {
        return display_;
    }

    virtual Backlight* GetBacklight() override
    {
        return backlight_;
    }

    virtual bool GetBatteryLevel(int& level, bool& charging, bool& discharging) override
    {
        static bool last_discharging = false;

        if (pmic_ == nullptr) {
            return false;
        }

        uint16_t vin_mv       = 0;
        uint8_t chg_stat      = 1;
        const bool vin_ok     = pmic_->readVin(&vin_mv) == M5PM1_OK;
        const bool chg_ok     = pmic_->gpioGetInput(PMG_CHG_STAT, &chg_stat) == M5PM1_OK;
        const bool ext_power  = vin_ok && vin_mv > 4000;
        const bool chg_active = chg_ok && chg_stat == 0;

        charging    = ext_power && chg_active;
        discharging = !ext_power;

        if (discharging != last_discharging) {
            power_save_timer_->SetEnabled(discharging);
            last_discharging = discharging;
        }

        uint16_t vbat_mv = 0;
        if (pmic_->readVbat(&vbat_mv) == M5PM1_OK) {
            battery_filtered_mv_ = (battery_filtered_mv_ == 0)
                                       ? vbat_mv
                                       : (uint16_t)((battery_filtered_mv_ * 7 + vbat_mv) / 8);
            level = BatteryMillivoltsToPercent(battery_filtered_mv_);
        }
        return true;
    }

    virtual void SetPowerSaveLevel(PowerSaveLevel level) override
    {
        if (level != PowerSaveLevel::LOW_POWER) {
            power_save_timer_->WakeUp();
        }
        WifiBoard::SetPowerSaveLevel(level);
    }
};

DECLARE_BOARD(M5StackStopWatch);
