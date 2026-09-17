file(GLOB_RECURSE MY_SRCS
    "apps/*.c"
    "apps/*.cc"
    "apps/*.cpp"
    "assets/*.c"
    "assets/*.cc"
    "assets/*.cpp"
    "hal/*.c"
    "hal/*.cc"
    "hal/*.cpp"
)

list(FILTER MY_SRCS EXCLUDE REGEX "(^|.*/)apps/app_alarm_clock/.*")
list(FILTER MY_SRCS EXCLUDE REGEX "(^|.*/)apps/app_stopwatch/.*")
list(FILTER MY_SRCS EXCLUDE REGEX "(^|.*/)assets/images/alarm_icon\\.c$")

list(APPEND MY_SRCS
    "apps/app_codex/app_codex.cpp"
    "apps/app_codex/codex_quota_client.cpp"
    "apps/app_codex/view/view.cpp"
    "apps/app_setup/workers/wifi.cpp"
    "apps/app_xiaozhi/app_xiaozhi.cpp"
    "assets/images/icon_codex.c"
    "assets/images/icon_xiaozhi.c"
    "hal/hal_bluetooth.cpp"
    "hal/codex_micro_hid.cpp"
)
list(REMOVE_DUPLICATES MY_SRCS)

set(MY_INCS
    "."
)

idf_component_register(
    SRCS 
        "main.cpp"
        ${MY_SRCS}
    INCLUDE_DIRS 
        ${MY_INCS}
    REQUIRES
        78__esp-wifi-connect
        ArduinoJson
        BMI270_BMM150_Sensor
        M5GFX
        M5IOE1
        M5PM1
        app_update
        esp_app_format
        bt
        esp_hid
        esp_pm
        esp-dsp
        esp_http_client
        esp_codec_dev
        fatfs
        i2c_bus
        lvgl
        mbedtls
        mooncake
        mooncake_log
        smooth_ui_toolkit
    EMBED_TXTFILES
        "hal/utils/config_ap/assets/badge_config_ap.html"
    EMBED_FILES
        "assets/sfx/boot_sfx.bin"
)
