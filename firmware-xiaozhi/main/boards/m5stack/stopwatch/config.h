#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

/*
 * M5Stack StopWatch (Tangaro) — ESP32-S3
 *
 * 硬件（引脚取自本机已量产验证的 StopWatch 固件 HAL，见 docs/xiaozhi-integration-plan.md）：
 *   - 显示   CO5300 AMOLED 466x466 (QSPI, panel 468x466 @ memory 480x480, gap x=6)
 *   - 音频   ES8311 (I2S 单声道; PA 由 M5IOE1/PY32 + GPIO14 双门控)
 *   - 电源   M5PM1 (I2C 0x6E)
 *   - IO 扩展 M5IOE1 / PY32 (I2C 0x4F 或 0x6F) — 屏幕 RST/使能、触摸 RST、喇叭 PA 都在这里
 *   - 触摸   CST820 (I2C)
 *   - I2C    SDA=47 SCL=48
 *
 * 注意：屏幕复位(OLED_RST)与使能(L3B_EN)不在 GPIO 上，必须通过 M5IOE1 操作，
 *       因此 stopwatch.cc 里 InitializeIOE() 必须在 InitializeDisplay() 之前完成。
 */

#define AUDIO_INPUT_SAMPLE_RATE  24000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

/* ---------------- I2S (ES8311) ---------------- */
#define AUDIO_I2S_GPIO_MCLK GPIO_NUM_18
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_17
#define AUDIO_I2S_GPIO_WS   GPIO_NUM_15
#define AUDIO_I2S_GPIO_DIN  GPIO_NUM_16   /* ADC -> ESP32 (麦克风) */
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_21   /* ESP32 -> DAC (喇叭)   */

/* 喇叭 PA 是双级门控：M5IOE1 的 PY32_SPK_PA_PIN 与 GPIO14 都要拉高。
 * GPIO14 交给小智的 AudioCodec 控制，IOE 那级在 InitializeIOE() 里常开。 */
#define AUDIO_CODEC_PA_PIN      GPIO_NUM_14
#define AUDIO_CODEC_ES8311_ADDR ES8311_CODEC_DEFAULT_ADDR

/* ---------------- I2C ---------------- */
#define AUDIO_CODEC_I2C_SDA_PIN GPIO_NUM_47
#define AUDIO_CODEC_I2C_SCL_PIN GPIO_NUM_48

/* ---------------- 显示 QSPI ---------------- */
#define EXAMPLE_PIN_NUM_LCD_CS    GPIO_NUM_39
#define EXAMPLE_PIN_NUM_LCD_PCLK  GPIO_NUM_40
#define EXAMPLE_PIN_NUM_LCD_DATA0 GPIO_NUM_41
#define EXAMPLE_PIN_NUM_LCD_DATA1 GPIO_NUM_42
#define EXAMPLE_PIN_NUM_LCD_DATA2 GPIO_NUM_46
#define EXAMPLE_PIN_NUM_LCD_DATA3 GPIO_NUM_45

/* 面板复位没有直连 GPIO，由 M5IOE1 的 PY32_OLED_RST_PIN 代管 */
#define EXAMPLE_PIN_NUM_LCD_RST GPIO_NUM_NC

#define DISPLAY_WIDTH  466
#define DISPLAY_HEIGHT 466

#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y false
#define DISPLAY_SWAP_XY  false

#define DISPLAY_OFFSET_X 0
#define DISPLAY_OFFSET_Y 0

/* CO5300 memory 480x480，可视面板居中偏移 x=6（沿用原固件 LGFX 的同一个值） */
#define DISPLAY_GAP_X 6
#define DISPLAY_GAP_Y 0

/* ---------------- 按键 ---------------- */
/* Button A 接 GPIO2，Button B 接 GPIO1；ESP32-S3 的 GPIO0 是 BOOT 键（未引出） */
#define USER_BUTTON_A_GPIO GPIO_NUM_2
#define USER_BUTTON_B_GPIO GPIO_NUM_1

/* 小智用它做“单击说话 / 启动阶段单击进配网” */
#define BOOT_BUTTON_GPIO USER_BUTTON_A_GPIO

#endif  // _BOARD_CONFIG_H_
