#include <Arduino.h>
#include <lvgl.h>
#include <RotaryEncoder.h>

#include "app_types.hpp"
#include "my_display.hpp"
#include "system_init.hpp"

namespace {

constexpr int ENC_CLK = 40;
constexpr int ENC_DT = 41;
constexpr int ENC_SW = 42;

constexpr uint32_t ENC_DEBOUNCE_MS = 30;
constexpr uint32_t ENC_LONG_PRESS_MS = 600;

constexpr uint32_t COLOR_BG = 0x101418;
constexpr uint32_t COLOR_ACCENT = 0x4FD1C5;
constexpr uint32_t COLOR_TEXT = 0xFFFFFF;
constexpr uint32_t COLOR_MUTED = 0xA0AEC0;
constexpr uint32_t COLOR_HINT = 0x718096;

LGFX gfx;
RotaryEncoder encoder(ENC_CLK, ENC_DT, RotaryEncoder::LatchMode::FOUR3);

long lastEncoderPos = 0;

bool encBtnPressed = false;
bool encBtnLastRaw = false;
bool encBtnLongPressDetected = false;
uint32_t encBtnLastChange = 0;
uint32_t encBtnPressStart = 0;

static lv_color_t draw_buf[SCR_W * BUF_LINES];

static lv_display_t *disp = nullptr;
static lv_indev_t *encoder_indev = nullptr;
static lv_group_t *encoder_group = nullptr;

ScreenId currentScreen = SCREEN_VOLUME_MASTER;
lv_obj_t *screens[NUM_SCREENS] = {nullptr};

AppVolume appVolumes[] = {
	{"Master", 100},
	{"Discord", 75},
	{"Spotify", 80},
	{"Browser", 60},
};

constexpr int NUM_APPS = sizeof(appVolumes) / sizeof(appVolumes[0]);
int selectedApp = 1;

lv_obj_t *volumeMasterSlider = nullptr;
lv_obj_t *volumeMasterLabel = nullptr;
lv_obj_t *appListLabel = nullptr;
lv_obj_t *appVolumeSlider = nullptr;
lv_obj_t *appVolumeLabel = nullptr;
lv_obj_t *systemInfoLabel = nullptr;

uint32_t bytes_to_kb(size_t value) {
	return static_cast<uint32_t>(value / 1024U);
}

void IRAM_ATTR encoderISR() {
	encoder.tick();
}

static uint32_t my_tick_get() {
	return millis();
}

void my_disp_flush(lv_display_t *display, const lv_area_t *area, uint8_t *px_map) {
	uint32_t width = area->x2 - area->x1 + 1;
	uint32_t height = area->y2 - area->y1 + 1;

	gfx.startWrite();
	gfx.setAddrWindow(area->x1, area->y1, width, height);
	gfx.writePixels(reinterpret_cast<lgfx::rgb565_t *>(px_map), width * height);
	gfx.endWrite();

	lv_display_flush_ready(display);
}

void encoder_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
	(void)indev;

	encoder.tick();

	long newPos = encoder.getPosition();
	int32_t delta = static_cast<int32_t>(newPos - lastEncoderPos);
	lastEncoderPos = newPos;

	data->enc_diff = static_cast<int16_t>(delta);
	data->state = LV_INDEV_STATE_RELEASED;
}

lv_obj_t *create_base_screen(const char *titleText, const char *hintText) {
	lv_obj_t *scr = lv_obj_create(nullptr);
	lv_obj_set_style_bg_color(scr, lv_color_hex(COLOR_BG), LV_PART_MAIN);
	lv_obj_set_style_border_width(scr, 0, LV_PART_MAIN);
	lv_obj_set_style_radius(scr, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_all(scr, 0, LV_PART_MAIN);

	lv_obj_t *title = lv_label_create(scr);
	lv_label_set_text(title, titleText);
	lv_obj_set_style_text_color(title, lv_color_hex(COLOR_ACCENT), LV_PART_MAIN);
	lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
	lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 8);

	lv_obj_t *hint = lv_label_create(scr);
	lv_label_set_text(hint, hintText);
	lv_obj_set_style_text_color(hint, lv_color_hex(COLOR_HINT), LV_PART_MAIN);
	lv_obj_set_style_text_font(hint, &lv_font_montserrat_10, LV_PART_MAIN);
	lv_obj_align(hint, LV_ALIGN_BOTTOM_LEFT, 4, -4);

	return scr;
}

void update_volume_master_screen() {
	lv_slider_set_value(volumeMasterSlider, appVolumes[0].volume, LV_ANIM_OFF);
	lv_label_set_text_fmt(volumeMasterLabel, "%d%%", appVolumes[0].volume);
}

void create_volume_master_screen() {
	lv_obj_t *scr = create_base_screen(
		"Master Volume",
		"Rotate: Volume\nShort: Mute\nLong: Next"
	);

	volumeMasterSlider = lv_slider_create(scr);
	lv_obj_set_width(volumeMasterSlider, 236);
	lv_obj_align(volumeMasterSlider, LV_ALIGN_TOP_MID, 0, 54);
	lv_slider_set_range(volumeMasterSlider, 0, 100);
	lv_obj_set_style_bg_color(volumeMasterSlider, lv_color_hex(COLOR_ACCENT), LV_PART_INDICATOR);

	volumeMasterLabel = lv_label_create(scr);
	lv_obj_set_style_text_color(volumeMasterLabel, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
	lv_obj_set_style_text_font(volumeMasterLabel, &lv_font_montserrat_32, LV_PART_MAIN);
	lv_obj_align(volumeMasterLabel, LV_ALIGN_CENTER, 0, 28);

	screens[SCREEN_VOLUME_MASTER] = scr;
	update_volume_master_screen();
}

void update_volume_apps_screen() {
	lv_label_set_text_fmt(
		appListLabel,
		"%s %s\n%s %s\n%s %s",
		(selectedApp == 1) ? "→" : " ", appVolumes[1].name,
		(selectedApp == 2) ? "→" : " ", appVolumes[2].name,
		(selectedApp == 3) ? "→" : " ", appVolumes[3].name
	);

	lv_slider_set_value(appVolumeSlider, appVolumes[selectedApp].volume, LV_ANIM_OFF);
	lv_label_set_text_fmt(appVolumeLabel, "%d%%", appVolumes[selectedApp].volume);
}

void create_volume_apps_screen() {
	lv_obj_t *scr = create_base_screen(
		"App Volume",
		"Rotate: Choose/Vol\nShort: Mute\nLong: Next"
	);

	lv_obj_t *appLabel = lv_label_create(scr);
	lv_label_set_text(appLabel, "Select app:");
	lv_obj_set_style_text_color(appLabel, lv_color_hex(COLOR_MUTED), LV_PART_MAIN);
	lv_obj_set_style_text_font(appLabel, &lv_font_montserrat_12, LV_PART_MAIN);
	lv_obj_align(appLabel, LV_ALIGN_TOP_LEFT, 16, 35);

	appListLabel = lv_label_create(scr);
	lv_obj_set_style_text_color(appListLabel, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
	lv_obj_set_style_text_font(appListLabel, &lv_font_montserrat_12, LV_PART_MAIN);
	lv_obj_align(appListLabel, LV_ALIGN_TOP_LEFT, 16, 56);

	lv_obj_t *volLabel = lv_label_create(scr);
	lv_label_set_text(volLabel, "Volume:");
	lv_obj_set_style_text_color(volLabel, lv_color_hex(COLOR_MUTED), LV_PART_MAIN);
	lv_obj_set_style_text_font(volLabel, &lv_font_montserrat_12, LV_PART_MAIN);
	lv_obj_align(volLabel, LV_ALIGN_TOP_LEFT, 16, 128);

	appVolumeLabel = lv_label_create(scr);
	lv_obj_set_style_text_color(appVolumeLabel, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
	lv_obj_set_style_text_font(appVolumeLabel, &lv_font_montserrat_12, LV_PART_MAIN);
	lv_obj_align(appVolumeLabel, LV_ALIGN_TOP_RIGHT, -16, 128);

	appVolumeSlider = lv_slider_create(scr);
	lv_obj_set_width(appVolumeSlider, 236);
	lv_obj_align(appVolumeSlider, LV_ALIGN_TOP_MID, 0, 152);
	lv_slider_set_range(appVolumeSlider, 0, 100);
	lv_obj_set_style_bg_color(appVolumeSlider, lv_color_hex(COLOR_ACCENT), LV_PART_INDICATOR);

	screens[SCREEN_VOLUME_APPS] = scr;
	update_volume_apps_screen();
}

void update_system_info_screen() {
	SystemMemoryInfo info = get_system_memory_info();

	lv_label_set_text_fmt(
		systemInfoLabel,
		"Heap total : %u KB\n"
		"Heap free  : %u KB\n"
		"Heap min   : %u KB\n"
		"Heap block : %u KB\n"
		"\n"
		"PSRAM      : %s\n"
		"PSRAM total: %u KB\n"
		"PSRAM free : %u KB\n"
		"PSRAM block: %u KB\n"
		"\n"
		"Flash size : %u KB\n"
		"Sketch size: %u KB\n"
		"Free space : %u KB",
		bytes_to_kb(info.heapTotal),
		bytes_to_kb(info.heapFree),
		bytes_to_kb(info.heapMinFree),
		bytes_to_kb(info.heapLargestBlock),
		info.psramAvailable ? "OK" : "N/A",
		bytes_to_kb(info.psramTotal),
		bytes_to_kb(info.psramFree),
		bytes_to_kb(info.psramLargestBlock),
		bytes_to_kb(info.flashSize),
		bytes_to_kb(info.sketchSize),
		bytes_to_kb(info.freeSketchSpace)
	);
}

void create_system_info_screen() {
	lv_obj_t *scr = create_base_screen(
		"System Memory",
		"Rotate: No action\nShort: Refresh\nLong: Next"
	);

	systemInfoLabel = lv_label_create(scr);
	lv_obj_set_width(systemInfoLabel, 248);
	lv_label_set_long_mode(systemInfoLabel, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_color(systemInfoLabel, lv_color_hex(COLOR_TEXT), LV_PART_MAIN);
	lv_obj_set_style_text_font(systemInfoLabel, &lv_font_montserrat_12, LV_PART_MAIN);
	lv_obj_align(systemInfoLabel, LV_ALIGN_TOP_LEFT, 16, 40);

	screens[SCREEN_SYSTEM_INFO] = scr;
	update_system_info_screen();
}

void switch_screen(ScreenId newScreen) {
	currentScreen = newScreen;
	lv_screen_load(screens[currentScreen]);

	if (currentScreen == SCREEN_VOLUME_MASTER) {
		update_volume_master_screen();
	} else if (currentScreen == SCREEN_VOLUME_APPS) {
		update_volume_apps_screen();
	} else if (currentScreen == SCREEN_SYSTEM_INFO) {
		update_system_info_screen();
	}

	Serial.print("Screen: ");
	Serial.println(static_cast<int>(currentScreen));
}

void splash_screen() {
	uint16_t colors[] = {
		0xF800,
		0x07E0,
		0x001F,
	};

	for (uint8_t i = 0; i < 3; ++i) {
		gfx.fillScreen(colors[i]);
		delay(250);
	}

	gfx.fillScreen(0x0000);
	delay(80);

	lv_obj_t *label = lv_label_create(lv_screen_active());
	lv_label_set_text(label, "HELLO)");
	lv_obj_set_style_text_color(label, lv_color_hex(COLOR_ACCENT), LV_PART_MAIN);
	lv_obj_set_style_text_font(label, &lv_font_montserrat_32, LV_PART_MAIN);
	lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
	lv_timer_handler();

	delay(1200);
	lv_obj_del(label);
}

void handle_short_press() {
	if (currentScreen == SCREEN_VOLUME_MASTER) {
		appVolumes[0].volume = (appVolumes[0].volume == 0) ? 100 : 0;
		update_volume_master_screen();
	} else if (currentScreen == SCREEN_VOLUME_APPS) {
		appVolumes[selectedApp].volume = (appVolumes[selectedApp].volume == 0) ? 100 : 0;
		update_volume_apps_screen();
	} else if (currentScreen == SCREEN_SYSTEM_INFO) {
		update_system_info_screen();
	}
}

void handle_rotation_delta(int32_t delta) {
	if (delta == 0) {
		return;
	}

	if (currentScreen == SCREEN_VOLUME_MASTER) {
		appVolumes[0].volume = constrain(appVolumes[0].volume + delta * 5, 0, 100);
		update_volume_master_screen();
		return;
	}

	if (currentScreen == SCREEN_VOLUME_APPS) {
		if (delta > 0) {
			selectedApp = (selectedApp % (NUM_APPS - 1)) + 1;
		} else {
			selectedApp = (selectedApp - 2 + (NUM_APPS - 1)) % (NUM_APPS - 1) + 1;
		}

		appVolumes[selectedApp].volume = constrain(appVolumes[selectedApp].volume + delta * 3, 0, 100);
		update_volume_apps_screen();
	}
}

void update_encoder_input() {
	encoder.tick();

	bool rawPressed = (digitalRead(ENC_SW) == LOW);
	uint32_t now = millis();

	if (rawPressed != encBtnLastRaw) {
		encBtnLastRaw = rawPressed;
		encBtnLastChange = now;
	}

	if ((now - encBtnLastChange) > ENC_DEBOUNCE_MS) {
		if (rawPressed && !encBtnPressed) {
			encBtnPressed = true;
			encBtnPressStart = now;
			encBtnLongPressDetected = false;
		} else if (!rawPressed && encBtnPressed) {
			if (!encBtnLongPressDetected && (now - encBtnPressStart) < ENC_LONG_PRESS_MS) {
				handle_short_press();
			}

			encBtnPressed = false;
			encBtnLongPressDetected = false;
		}
	}

	if (encBtnPressed && !encBtnLongPressDetected && (now - encBtnPressStart) > ENC_LONG_PRESS_MS) {
		encBtnLongPressDetected = true;
		ScreenId nextScreen = static_cast<ScreenId>((static_cast<int>(currentScreen) + 1) % NUM_SCREENS);
		switch_screen(nextScreen);
	}

	long newPos = encoder.getPosition();
	int32_t delta = static_cast<int32_t>(newPos - lastEncoderPos);

	if (delta != 0) {
		lastEncoderPos = newPos;
		handle_rotation_delta(delta);
	}
}

}  // namespace

void setup() {
	Serial.begin(115200);
	delay(800);
	Serial.println("\n=== Phase 1: Volume Controller UI ===");

	init_system_memory();
	print_system_memory_info();

	gfx.init();
	gfx.setRotation(1);
	gfx.setBrightness(255);

	pinMode(ENC_CLK, INPUT_PULLUP);
	pinMode(ENC_DT, INPUT_PULLUP);
	pinMode(ENC_SW, INPUT_PULLUP);

	encoder.tick();
	lastEncoderPos = encoder.getPosition();

	attachInterrupt(digitalPinToInterrupt(ENC_CLK), encoderISR, CHANGE);
	attachInterrupt(digitalPinToInterrupt(ENC_DT), encoderISR, CHANGE);

	lv_init();
	lv_tick_set_cb(my_tick_get);

	disp = lv_display_create(SCR_W, SCR_H);
	lv_display_set_flush_cb(disp, my_disp_flush);
	lv_display_set_buffers(disp, draw_buf, nullptr, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

	encoder_group = lv_group_create();
	lv_group_set_default(encoder_group);

	encoder_indev = lv_indev_create();
	lv_indev_set_type(encoder_indev, LV_INDEV_TYPE_ENCODER);
	lv_indev_set_read_cb(encoder_indev, encoder_read_cb);
	lv_indev_set_group(encoder_indev, encoder_group);

	splash_screen();

	create_volume_master_screen();
	create_volume_apps_screen();
	create_system_info_screen();

	switch_screen(SCREEN_VOLUME_MASTER);

	Serial.println("Setup complete.");
}

void loop() {
	update_encoder_input();
	lv_timer_handler();
	delay(5);
}
