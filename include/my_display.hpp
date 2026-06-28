#pragma once

// =========================================================
// Конфигурация LovyanGFX для ST7789V3 280x240 (landscape) на ESP32-S3.
// Распиновка - финальная, проверенная на железе:
//   SCLK=12, MOSI=11 (дефолтный аппаратный SPI2/FSPI)
//   RST=17, DC=18, CS=10, BLK=8 (HIGH = подсветка включена)
// =========================================================

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel_instance;
  lgfx::Bus_SPI _bus_instance;
  lgfx::Light_PWM _light_instance;

public:
  LGFX(void) {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = SPI2_HOST;   // FSPI на esp32-s3
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;  // 40MHz - надёжный старт; можно поднять позже
      cfg.freq_read  = 16000000;
      cfg.spi_3wire  = false;
      cfg.use_lock   = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = 12;
      cfg.pin_mosi = 11;
      cfg.pin_miso = -1;
      cfg.pin_dc   = 18;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs    = 10;
      cfg.pin_rst   = 17;
      cfg.pin_busy  = -1;

      cfg.memory_width  = 240;
      cfg.memory_height = 320;  // физический буфер контроллера ST7789
      cfg.panel_width    = 240;
      cfg.panel_height   = 280; // физическая видимая область до поворота
      cfg.offset_x = 0;
      cfg.offset_y = 20;         // проверенное смещение для этой панели
      cfg.offset_rotation = 0;

      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable   = false;
      cfg.invert     = true;    // часто нужно для ST7789, поправим если цвета неверные
      cfg.rgb_order  = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;

      _panel_instance.config(cfg);
    }

    {
      // Подсветка через обычный PWM-канал. На этом модуле BLK НЕ инвертирована:
      // HIGH = включена. invert=false соответствует этому.
      auto cfg = _light_instance.config();
      cfg.pin_bl = 8;
      cfg.invert = false;
      cfg.freq   = 12000;
      cfg.pwm_channel = 7;
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    setPanel(&_panel_instance);
  }
};