#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <esp_spi_flash.h>

#if __has_include(<esp_psram.h>)
#include <esp_psram.h>
#endif

#include "system_init.hpp"

namespace {

void print_bytes_kb(const char *label, size_t value) {
  Serial.print(label);
  Serial.print(value);
  Serial.print(" bytes (");
  Serial.print(value / 1024);
  Serial.println(" KB)");
}

} // namespace

void init_system_memory() {
  Serial.println("[SYS] Early memory init...");

#if defined(BOARD_HAS_PSRAM)
  bool psramOk = psramInit();
  Serial.print("[SYS] PSRAM init: ");
  Serial.println(psramOk ? "OK" : "FAILED");
#else
  Serial.println("[SYS] PSRAM init: skipped (BOARD_HAS_PSRAM not defined)");
#endif
}

void print_system_memory_info() {
  Serial.println("[SYS] Memory / flash info:");

  print_bytes_kb("  Heap total: ", ESP.getHeapSize());
  print_bytes_kb("  Heap free : ", ESP.getFreeHeap());
  print_bytes_kb("  Heap min  : ", ESP.getMinFreeHeap());
  print_bytes_kb("  PSRAM size: ", ESP.getPsramSize());
  print_bytes_kb("  PSRAM free: ", ESP.getFreePsram());

  Serial.print("  Flash size: ");
  Serial.print(ESP.getFlashChipSize());
  Serial.println(" bytes");

  Serial.print("  Sketch size: ");
  Serial.print(ESP.getSketchSize());
  Serial.println(" bytes");

  Serial.print("  Free sketch space: ");
  Serial.print(ESP.getFreeSketchSpace());
  Serial.println(" bytes");

  Serial.print("  Largest 8-bit heap block: ");
  Serial.println(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));

#if defined(BOARD_HAS_PSRAM)
  Serial.print("  Largest PSRAM block: ");
  Serial.println(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
#endif
}
