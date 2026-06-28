#pragma once

#include <Arduino.h>
#include <lvgl.h>

constexpr uint32_t SCR_W = 280;
constexpr uint32_t SCR_H = 240;
constexpr uint32_t BUF_LINES = 30;

// =========================================================
// Экраны приложения
// =========================================================
enum ScreenId {
  SCREEN_VOLUME_MASTER = 0,
  SCREEN_VOLUME_APPS = 1,
  SCREEN_DEVICES_OUTPUT = 2,
  SCREEN_DEVICES_INPUT = 3,
  NUM_SCREENS = 4
};

// =========================================================
// Демо-данные
// =========================================================
struct AppVolume {
  const char *name;
  int32_t volume;
};

struct Device {
  const char *name;
};
