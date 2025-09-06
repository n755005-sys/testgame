// pomodoro_icons.h — shim to avoid duplicate definitions
#pragma once
#include <Arduino.h>

#ifndef POMODORO_ICON_W
#define POMODORO_ICON_W 32
#define POMODORO_ICON_H 32
#endif

// 這裡只做 extern 宣告；真正的資料由 pixel_icons_rgb565.h 提供
extern const uint16_t I_Play_btn[];
extern const uint16_t I_Pause_btn[];
extern const uint16_t I_Plus_btn[];
extern const uint16_t I_Tomato_finish[];
