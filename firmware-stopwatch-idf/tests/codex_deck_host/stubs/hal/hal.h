#pragma once
#include <lvgl.h>
#include <cstdint>
struct MockHal {
    lv_indev_t* lvTouchpad = nullptr;
    struct Imu { float gyroX = 0, gyroY = 0, gyroZ = 0; } imu;
    uint32_t millis() { return lv_tick_get(); }
    uint8_t batteryLevel = 78;
    uint8_t getBatteryLevel() { return batteryLevel; }
    void updateImuData() {}
    const Imu& getImuData() { return imu; }
    void vibrate(int, int) {}
};
inline MockHal& GetHAL() { static MockHal hal; return hal; }
