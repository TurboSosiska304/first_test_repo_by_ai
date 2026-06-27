#include <Arduino.h>
#include <lvgl.h>
#include "my_display.hpp"

LGFX gfx;

// =========================================================
// Энкодер KY-040
// =========================================================
#define ENC_CLK 40
#define ENC_DT  41
#define ENC_SW  42

volatile int32_t encDelta = 0;
volatile uint8_t lastClkState = 0;

void IRAM_ATTR encoderISR() {
  uint8_t clk = digitalRead(ENC_CLK);
  if (clk != lastClkState) {
    if (digitalRead(ENC_DT) != clk) {
      encDelta++;
    } else {
      encDelta--;
    }
  }
  lastClkState = clk;
}

bool encBtnPressed = false;
bool encBtnLastRaw = false;
uint32_t encBtnLastChange = 0;
uint32_t encBtnPressStart = 0;
const uint32_t ENC_DEBOUNCE_MS = 30;
const uint32_t ENC_LONG_PRESS_MS = 600; // скоро будет долгое нажатие

bool encBtnLongPressDetected = false;

static const uint32_t SCR_W = 280;
static const uint32_t SCR_H = 240;
static const uint32_t BUF_LINES = 30;
static lv_color_t draw_buf[SCR_W * BUF_LINES];

static lv_display_t *disp;
static lv_indev_t *encoder_indev;
static lv_group_t *encoder_group;

// =========================================================
// Экраны (вкладки)
// =========================================================
enum ScreenId {
  SCREEN_VOLUME_MASTER = 0,
  SCREEN_VOLUME_APPS = 1,
  SCREEN_DEVICES_OUTPUT = 2,
  SCREEN_DEVICES_INPUT = 3,
  SCREEN_PLAYBACK = 4,
  SCREEN_SYSINFO = 5,
  NUM_SCREENS = 6
};

ScreenId currentScreen = SCREEN_VOLUME_MASTER;
lv_obj_t *screens[NUM_SCREENS];

// =========================================================
// Демо-данные
// =========================================================
struct AppVolume {
  const char *name;
  int32_t volume;
};

AppVolume appVolumes[] = {
  {"Master", 100},
  {"Discord", 75},
  {"Spotify", 80},
  {"Browser", 60},
};
const int NUM_APPS = 4;
int selectedApp = 1; // начнём с Discord, не Master

struct PlaybackInfo {
  const char *artist;
  const char *track;
  int32_t position;
  int32_t duration;
  bool isPlaying;
};

PlaybackInfo playback = {
  "The Weeknd",
  "Blinding Lights",
  45,
  200,
  true
};

struct Device {
  const char *name;
};

Device outputDevices[] = {
  {"Speakers"},
  {"Headphones"},
};
const int NUM_OUTPUT_DEVICES = 2;
int selectedOutput = 0;

Device inputDevices[] = {
  {"Mic 1"},
  {"Mic 2"},
};
const int NUM_INPUT_DEVICES = 2;
int selectedInput = 0;

struct SysInfo {
  float cpuLoad;
  float gpuLoad;
  int32_t ramUsed;
  int32_t ramTotal;
  int32_t tempCPU;
};

SysInfo sysInfo = {
  45.5f, 12.0f, 4096, 8192, 52
};

// =========================================================
// LVGL Display & Input
// =========================================================
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;

  gfx.startWrite();
  gfx.setAddrWindow(area->x1, area->y1, w, h);
  gfx.writePixels((lgfx::rgb565_t *)px_map, w * h);
  gfx.endWrite();

  lv_display_flush_ready(disp);
}

static uint32_t my_tick_get(void) {
  return millis();
}

static void encoder_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
  noInterrupts();
  int32_t delta = encDelta;
  encDelta = 0;
  interrupts();

  data->enc_diff = (int16_t)delta;
  data->state = LV_INDEV_STATE_RELEASED;
}

// =========================================================
// Screen 0: Volume Master
// =========================================================
lv_obj_t *volumeMasterSlider;
lv_obj_t *volumeMasterLabel;

void create_volume_master_screen() {
  lv_obj_t *scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), LV_PART_MAIN);

  lv_obj_t *title = lv_label_create(scr);
  lv_label_set_text(title, "Master Volume");
  lv_obj_set_style_text_color(title, lv_color_hex(0x4FD1C5), LV_PART_MAIN);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

  volumeMasterSlider = lv_slider_create(scr);
  lv_obj_set_width(volumeMasterSlider, 236);
  lv_obj_align(volumeMasterSlider, LV_ALIGN_TOP_MID, 0, 54);
  lv_slider_set_range(volumeMasterSlider, 0, 100);
  lv_slider_set_value(volumeMasterSlider, appVolumes[0].volume, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(volumeMasterSlider, lv_color_hex(0x4FD1C5), LV_PART_INDICATOR);

  volumeMasterLabel = lv_label_create(scr);
  lv_label_set_text_fmt(volumeMasterLabel, "%d%%", appVolumes[0].volume);
  lv_obj_set_style_text_color(volumeMasterLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_font(volumeMasterLabel, &lv_font_montserrat_32, LV_PART_MAIN);
  lv_obj_align(volumeMasterLabel, LV_ALIGN_CENTER, 0, 28);

  lv_obj_t *hint = lv_label_create(scr);
  lv_label_set_text(hint, "Rotate: Volume\nShort: Mute\nLong: Next");
  lv_obj_set_style_text_color(hint, lv_color_hex(0x718096), LV_PART_MAIN);
  lv_obj_set_style_text_font(hint, &lv_font_montserrat_10, LV_PART_MAIN);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_LEFT, 4, -4);

  screens[SCREEN_VOLUME_MASTER] = scr;
}

void update_volume_master_screen() {
  lv_slider_set_value(volumeMasterSlider, appVolumes[0].volume, LV_ANIM_OFF);
  lv_label_set_text_fmt(volumeMasterLabel, "%d%%", appVolumes[0].volume);
}

// =========================================================
// Screen 1: Volume Apps
// =========================================================
lv_obj_t *appListLabel;
lv_obj_t *appVolumeSlider;
lv_obj_t *appVolumeLabel;

void create_volume_apps_screen() {
  lv_obj_t *scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), LV_PART_MAIN);

  lv_obj_t *title = lv_label_create(scr);
  lv_label_set_text(title, "App Volume");
  lv_obj_set_style_text_color(title, lv_color_hex(0x4FD1C5), LV_PART_MAIN);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

  lv_obj_t *appLabel = lv_label_create(scr);
  lv_label_set_text(appLabel, "Select app:");
  lv_obj_set_style_text_color(appLabel, lv_color_hex(0xA0AEC0), LV_PART_MAIN);
  lv_obj_align(appLabel, LV_ALIGN_TOP_LEFT, 16, 35);

  // App list с стрелочкой для выбора
  appListLabel = lv_label_create(scr);
  lv_obj_set_style_text_color(appListLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_font(appListLabel, &lv_font_montserrat_12, LV_PART_MAIN);
  lv_obj_align(appListLabel, LV_ALIGN_TOP_LEFT, 16, 52);

  // Volume slider
  lv_obj_t *volLabel = lv_label_create(scr);
  lv_label_set_text(volLabel, "Volume:");
  lv_obj_set_style_text_color(volLabel, lv_color_hex(0xA0AEC0), LV_PART_MAIN);
  lv_obj_align(volLabel, LV_ALIGN_TOP_LEFT, 16, 122);

  appVolumeSlider = lv_slider_create(scr);
  lv_obj_set_width(appVolumeSlider, 236);
  lv_obj_align(appVolumeSlider, LV_ALIGN_TOP_MID, 0, 144);
  lv_slider_set_range(appVolumeSlider, 0, 100);
  lv_obj_set_style_bg_color(appVolumeSlider, lv_color_hex(0x4FD1C5), LV_PART_INDICATOR);

  appVolumeLabel = lv_label_create(scr);
  lv_obj_set_style_text_color(appVolumeLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_align(appVolumeLabel, LV_ALIGN_TOP_RIGHT, -16, 122);

  lv_obj_t *hint = lv_label_create(scr);
  lv_label_set_text(hint, "Rotate: Choose/Vol\nShort: Mute\nLong: Next");
  lv_obj_set_style_text_color(hint, lv_color_hex(0x718096), LV_PART_MAIN);
  lv_obj_set_style_text_font(hint, &lv_font_montserrat_10, LV_PART_MAIN);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_LEFT, 4, -4);

  screens[SCREEN_VOLUME_APPS] = scr;
}

void update_volume_apps_screen() {
  // Отобразим приложения кроме Master (индексы 1-3)
  lv_label_set_text_fmt(appListLabel,
    "%s %s\n%s %s\n%s %s",
    (selectedApp == 1) ? "→" : " ", appVolumes[1].name,
    (selectedApp == 2) ? "→" : " ", appVolumes[2].name,
    (selectedApp == 3) ? "→" : " ", appVolumes[3].name
  );

  lv_slider_set_value(appVolumeSlider, appVolumes[selectedApp].volume, LV_ANIM_OFF);
  lv_label_set_text_fmt(appVolumeLabel, "%d%%", appVolumes[selectedApp].volume);
}

// =========================================================
// Screen 2: Devices Output
// =========================================================
lv_obj_t *outputListLabel;

void create_devices_output_screen() {
  lv_obj_t *scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), LV_PART_MAIN);

  lv_obj_t *title = lv_label_create(scr);
  lv_label_set_text(title, "Output Device");
  lv_obj_set_style_text_color(title, lv_color_hex(0x4FD1C5), LV_PART_MAIN);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

  outputListLabel = lv_label_create(scr);
  lv_obj_set_style_text_color(outputListLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_font(outputListLabel, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_obj_align(outputListLabel, LV_ALIGN_TOP_LEFT, 24, 52);

  lv_obj_t *hint = lv_label_create(scr);
  lv_label_set_text(hint, "Rotate: Select\nShort: Confirm\nLong: Next");
  lv_obj_set_style_text_color(hint, lv_color_hex(0x718096), LV_PART_MAIN);
  lv_obj_set_style_text_font(hint, &lv_font_montserrat_10, LV_PART_MAIN);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_LEFT, 4, -4);

  screens[SCREEN_DEVICES_OUTPUT] = scr;
}

void update_devices_output_screen() {
  lv_label_set_text_fmt(outputListLabel,
    "%s %s\n\n%s %s",
    (selectedOutput == 0) ? "→" : " ", outputDevices[0].name,
    (selectedOutput == 1) ? "→" : " ", outputDevices[1].name
  );
}

// =========================================================
// Screen 3: Devices Input
// =========================================================
lv_obj_t *inputListLabel;

void create_devices_input_screen() {
  lv_obj_t *scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), LV_PART_MAIN);

  lv_obj_t *title = lv_label_create(scr);
  lv_label_set_text(title, "Input Device");
  lv_obj_set_style_text_color(title, lv_color_hex(0x4FD1C5), LV_PART_MAIN);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

  inputListLabel = lv_label_create(scr);
  lv_obj_set_style_text_color(inputListLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_font(inputListLabel, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_obj_align(inputListLabel, LV_ALIGN_TOP_LEFT, 24, 52);

  lv_obj_t *hint = lv_label_create(scr);
  lv_label_set_text(hint, "Rotate: Select\nShort: Confirm\nLong: Next");
  lv_obj_set_style_text_color(hint, lv_color_hex(0x718096), LV_PART_MAIN);
  lv_obj_set_style_text_font(hint, &lv_font_montserrat_10, LV_PART_MAIN);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_LEFT, 4, -4);

  screens[SCREEN_DEVICES_INPUT] = scr;
}

void update_devices_input_screen() {
  lv_label_set_text_fmt(inputListLabel,
    "%s %s\n\n%s %s",
    (selectedInput == 0) ? "→" : " ", inputDevices[0].name,
    (selectedInput == 1) ? "→" : " ", inputDevices[1].name
  );
}

// =========================================================
// Screen 4: Playback
// =========================================================
lv_obj_t *trackLabel;
lv_obj_t *artistLabel;
lv_obj_t *playPauseBtn;
lv_obj_t *timeLabel;

void create_playback_screen() {
  lv_obj_t *scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), LV_PART_MAIN);

  lv_obj_t *title = lv_label_create(scr);
  lv_label_set_text(title, "Playback");
  lv_obj_set_style_text_color(title, lv_color_hex(0x4FD1C5), LV_PART_MAIN);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

  lv_obj_t *infoBox = lv_obj_create(scr);
  lv_obj_set_width(infoBox, 248);
  lv_obj_set_height(infoBox, 88);
  lv_obj_align(infoBox, LV_ALIGN_TOP_MID, 0, 34);
  lv_obj_set_style_bg_color(infoBox, lv_color_hex(0x1A202C), LV_PART_MAIN);
  lv_obj_set_style_border_color(infoBox, lv_color_hex(0x4FD1C5), LV_PART_MAIN);
  lv_obj_set_style_border_width(infoBox, 2, LV_PART_MAIN);

  trackLabel = lv_label_create(infoBox);
  lv_label_set_text(trackLabel, "");
  lv_obj_set_style_text_color(trackLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_font(trackLabel, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_obj_align(trackLabel, LV_ALIGN_TOP_MID, 0, 4);

  artistLabel = lv_label_create(infoBox);
  lv_label_set_text(artistLabel, "");
  lv_obj_set_style_text_color(artistLabel, lv_color_hex(0xA0AEC0), LV_PART_MAIN);
  lv_obj_set_style_text_font(artistLabel, &lv_font_montserrat_12, LV_PART_MAIN);
  lv_obj_align(artistLabel, LV_ALIGN_TOP_MID, 0, 24);

  timeLabel = lv_label_create(infoBox);
  lv_label_set_text(timeLabel, "");
  lv_obj_set_style_text_color(timeLabel, lv_color_hex(0x718096), LV_PART_MAIN);
  lv_obj_set_style_text_font(timeLabel, &lv_font_montserrat_10, LV_PART_MAIN);
  lv_obj_align(timeLabel, LV_ALIGN_BOTTOM_MID, 0, -4);

  playPauseBtn = lv_button_create(scr);
  lv_obj_set_width(playPauseBtn, 110);
  lv_obj_set_height(playPauseBtn, 40);
  lv_obj_align(playPauseBtn, LV_ALIGN_CENTER, -64, 46);
  lv_obj_t *label = lv_label_create(playPauseBtn);
  lv_label_set_text(label, "Play");
  lv_obj_set_style_bg_color(playPauseBtn, lv_color_hex(0x4FD1C5), LV_PART_MAIN);

  lv_obj_t *nextBtn = lv_button_create(scr);
  lv_obj_set_width(nextBtn, 110);
  lv_obj_set_height(nextBtn, 40);
  lv_obj_align(nextBtn, LV_ALIGN_CENTER, 64, 46);
  label = lv_label_create(nextBtn);
  lv_label_set_text(label, "Next");
  lv_obj_set_style_bg_color(nextBtn, lv_color_hex(0x4FD1C5), LV_PART_MAIN);

  lv_obj_t *hint = lv_label_create(scr);
  lv_label_set_text(hint, "Rotate: Seek\nShort: Play/Pause\nLong: Next");
  lv_obj_set_style_text_color(hint, lv_color_hex(0x718096), LV_PART_MAIN);
  lv_obj_set_style_text_font(hint, &lv_font_montserrat_10, LV_PART_MAIN);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_LEFT, 4, -4);

  screens[SCREEN_PLAYBACK] = scr;
}

void update_playback_screen() {
  lv_label_set_text(trackLabel, playback.track);
  lv_label_set_text(artistLabel, playback.artist);
  
  int mins = playback.position / 60;
  int secs = playback.position % 60;
  int dmins = playback.duration / 60;
  int dsecs = playback.duration % 60;
  lv_label_set_text_fmt(timeLabel, "%d:%02d / %d:%02d",
    mins, secs, dmins, dsecs);

  lv_label_set_text(lv_obj_get_child(playPauseBtn, 0),
    playback.isPlaying ? "Pause" : "Play");
}

// =========================================================
// Screen 5: System Info
// =========================================================
lv_obj_t *cpuLabel;
lv_obj_t *ramLabel;
lv_obj_t *tempLabel;

void create_sysinfo_screen() {
  lv_obj_t *scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), LV_PART_MAIN);

  lv_obj_t *title = lv_label_create(scr);
  lv_label_set_text(title, "System Info");
  lv_obj_set_style_text_color(title, lv_color_hex(0x4FD1C5), LV_PART_MAIN);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

  lv_obj_t *cpuTitleLabel = lv_label_create(scr);
  lv_label_set_text(cpuTitleLabel, "CPU:");
  lv_obj_set_style_text_color(cpuTitleLabel, lv_color_hex(0xA0AEC0), LV_PART_MAIN);
  lv_obj_align(cpuTitleLabel, LV_ALIGN_TOP_LEFT, 20, 36);

  cpuLabel = lv_label_create(scr);
  lv_obj_set_style_text_color(cpuLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_align(cpuLabel, LV_ALIGN_TOP_LEFT, 20, 54);

  lv_obj_t *ramTitleLabel = lv_label_create(scr);
  lv_label_set_text(ramTitleLabel, "RAM:");
  lv_obj_set_style_text_color(ramTitleLabel, lv_color_hex(0xA0AEC0), LV_PART_MAIN);
  lv_obj_align(ramTitleLabel, LV_ALIGN_TOP_LEFT, 20, 86);

  ramLabel = lv_label_create(scr);
  lv_obj_set_style_text_color(ramLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_align(ramLabel, LV_ALIGN_TOP_LEFT, 20, 104);

  lv_obj_t *tempTitleLabel = lv_label_create(scr);
  lv_label_set_text(tempTitleLabel, "Temperature:");
  lv_obj_set_style_text_color(tempTitleLabel, lv_color_hex(0xA0AEC0), LV_PART_MAIN);
  lv_obj_align(tempTitleLabel, LV_ALIGN_TOP_LEFT, 20, 136);

  tempLabel = lv_label_create(scr);
  lv_obj_set_style_text_color(tempLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_align(tempLabel, LV_ALIGN_TOP_LEFT, 20, 154);

  lv_obj_t *hint = lv_label_create(scr);
  lv_label_set_text(hint, "Long press: Next tab");
  lv_obj_set_style_text_color(hint, lv_color_hex(0x718096), LV_PART_MAIN);
  lv_obj_set_style_text_font(hint, &lv_font_montserrat_10, LV_PART_MAIN);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_LEFT, 4, -4);

  screens[SCREEN_SYSINFO] = scr;
}

void update_sysinfo_screen() {
  lv_label_set_text_fmt(cpuLabel, "%.1f%% / GPU: %.1f%%", sysInfo.cpuLoad, sysInfo.gpuLoad);
  lv_label_set_text_fmt(ramLabel, "%d / %d MB", sysInfo.ramUsed, sysInfo.ramTotal);
  lv_label_set_text_fmt(tempLabel, "%d°C", sysInfo.tempCPU);
}

// =========================================================
// Навигация между экранами
// =========================================================
void switch_screen(ScreenId newScreen) {
  currentScreen = newScreen;
  lv_screen_load(screens[currentScreen]);
  
  if (currentScreen == SCREEN_VOLUME_MASTER) update_volume_master_screen();
  else if (currentScreen == SCREEN_VOLUME_APPS) update_volume_apps_screen();
  else if (currentScreen == SCREEN_DEVICES_OUTPUT) update_devices_output_screen();
  else if (currentScreen == SCREEN_DEVICES_INPUT) update_devices_input_screen();
  else if (currentScreen == SCREEN_PLAYBACK) update_playback_screen();
  else if (currentScreen == SCREEN_SYSINFO) update_sysinfo_screen();

  Serial.print("Screen: ");
  Serial.println(currentScreen);
}

// =========================================================
// Splash screen
// =========================================================
void splash_screen() {
  uint16_t colors[] = {
    0xF800, // red
    0x07E0, // green
    0x001F, // blue
    0xFFE0, // yellow
    0xF81F, // magenta
    0x07FF, // cyan
  };

  for (int i = 0; i < 3; i++) {
    gfx.fillScreen(colors[i]);
    delay(300);
  }

  gfx.fillScreen(0x0000); // black
  delay(100);

  // Выводим "HELLO)" большими буквами в центре
  lv_obj_t *label = lv_label_create(lv_screen_active());
  lv_label_set_text(label, "HELLO)");
  lv_obj_set_style_text_color(label, lv_color_hex(0x4FD1C5), LV_PART_MAIN);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_32, LV_PART_MAIN);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_timer_handler();

  delay(1500);

  lv_obj_del(label);
}

// =========================================================
// Опрос энкодера + обработка событий
// =========================================================
void update_encoder_input() {
  // Дебаунс кнопки
  bool rawPressed = (digitalRead(ENC_SW) == LOW);
  uint32_t now = millis();

  if (rawPressed != encBtnLastRaw) {
    encBtnLastChange = now;
    encBtnLastRaw = rawPressed;
  }

  if (now - encBtnLastChange > ENC_DEBOUNCE_MS) {
    if (rawPressed && !encBtnPressed) {
      // Нажата
      encBtnPressStart = now;
      encBtnPressed = true;
      encBtnLongPressDetected = false;
    } else if (!rawPressed && encBtnPressed) {
      // Отпущена
      uint32_t pressDuration = now - encBtnPressStart;
      if (!encBtnLongPressDetected && pressDuration < ENC_LONG_PRESS_MS) {
        // Было короткое нажатие
        Serial.println("Short press");
        
        if (currentScreen == SCREEN_VOLUME_MASTER) {
          appVolumes[0].volume = (appVolumes[0].volume == 0) ? 100 : 0;
          update_volume_master_screen();
        }
        else if (currentScreen == SCREEN_VOLUME_APPS) {
          appVolumes[selectedApp].volume = (appVolumes[selectedApp].volume == 0) ? 100 : 0;
          update_volume_apps_screen();
        }
        else if (currentScreen == SCREEN_PLAYBACK) {
          playback.isPlaying = !playback.isPlaying;
          update_playback_screen();
        }
      }
      encBtnPressed = false;
      encBtnLongPressDetected = false;
    }
  }

  // Проверяем долгое нажатие ДО отпускания
  if (encBtnPressed && !encBtnLongPressDetected) {
    uint32_t pressDuration = millis() - encBtnPressStart;
    if (pressDuration > ENC_LONG_PRESS_MS) {
      // Долгое нажатие обнаружено - СРАЗУ переходим
      encBtnLongPressDetected = true;
      Serial.println("Long press detected - switch tab");
      currentScreen = (ScreenId)((currentScreen + 1) % NUM_SCREENS);
      switch_screen(currentScreen);
    }
  }

  // Вращение энкодера
  noInterrupts();
  int32_t delta = encDelta;
  encDelta = 0;
  interrupts();

  if (delta != 0) {
    if (currentScreen == SCREEN_VOLUME_MASTER) {
      appVolumes[0].volume = constrain(appVolumes[0].volume + delta * 5, 0, 100);
      update_volume_master_screen();
    }
    else if (currentScreen == SCREEN_VOLUME_APPS) {
      // При вращении вверх - выбираем предыдущее приложение, вниз - следующее
      if (delta > 0) {
        selectedApp = (selectedApp + 1) % (NUM_APPS - 1) + 1; // циклим в диапазоне 1-3
      } else {
        selectedApp = (selectedApp - 1 + NUM_APPS - 1) % (NUM_APPS - 1) + 1;
      }
      appVolumes[selectedApp].volume = constrain(appVolumes[selectedApp].volume + delta * 3, 0, 100);
      update_volume_apps_screen();
    }
    else if (currentScreen == SCREEN_DEVICES_OUTPUT) {
      selectedOutput = (selectedOutput + delta + NUM_OUTPUT_DEVICES) % NUM_OUTPUT_DEVICES;
      update_devices_output_screen();
    }
    else if (currentScreen == SCREEN_DEVICES_INPUT) {
      selectedInput = (selectedInput + delta + NUM_INPUT_DEVICES) % NUM_INPUT_DEVICES;
      update_devices_input_screen();
    }
    else if (currentScreen == SCREEN_PLAYBACK) {
      playback.position = constrain(playback.position + delta * 2, 0, playback.duration);
      update_playback_screen();
    }
    else if (currentScreen == SCREEN_SYSINFO) {
      sysInfo.cpuLoad = constrain(sysInfo.cpuLoad + delta * 2.5f, 0.0f, 100.0f);
      update_sysinfo_screen();
    }
  }
}

// =========================================================
// Setup & Loop
// =========================================================
void setup() {
  Serial.begin(115200);
  delay(800);
  Serial.println("\n=== ФАЗА 1 v2: Multi-screen Volume Controller (6 tabs) ===");

  gfx.init();
  gfx.setRotation(1);
  gfx.setBrightness(255);

  // Энкодер
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);

  lastClkState = digitalRead(ENC_CLK);
  attachInterrupt(digitalPinToInterrupt(ENC_CLK), encoderISR, CHANGE);

  // LVGL
  lv_init();
  lv_tick_set_cb(my_tick_get);

  disp = lv_display_create(SCR_W, SCR_H);
  lv_display_set_flush_cb(disp, my_disp_flush);
  lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

  encoder_group = lv_group_create();
  lv_group_set_default(encoder_group);

  encoder_indev = lv_indev_create();
  lv_indev_set_type(encoder_indev, LV_INDEV_TYPE_ENCODER);
  lv_indev_set_read_cb(encoder_indev, encoder_read_cb);
  lv_indev_set_group(encoder_indev, encoder_group);

  // Splash screen
  splash_screen();

  // Создаём все экраны
  Serial.println("Creating screens...");
  create_volume_master_screen();
  create_volume_apps_screen();
  create_devices_output_screen();
  create_devices_input_screen();
  create_playback_screen();
  create_sysinfo_screen();

  // Загружаем первый экран
  switch_screen(SCREEN_VOLUME_MASTER);

  Serial.println("Setup complete. Ready to navigate!");
}

void loop() {
  update_encoder_input();
  lv_timer_handler();
  delay(5);
}