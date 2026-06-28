#include <Arduino.h>
#include <esp_heap_caps.h>
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

}  // namespace

SystemMemoryInfo get_system_memory_info() {
	SystemMemoryInfo info{};

#if defined(BOARD_HAS_PSRAM)
	info.psramAvailable = (ESP.getPsramSize() > 0);
#else
	info.psramAvailable = false;
#endif

	info.heapTotal = ESP.getHeapSize();
	info.heapFree = ESP.getFreeHeap();
	info.heapMinFree = ESP.getMinFreeHeap();
	info.heapLargestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
	info.psramTotal = ESP.getPsramSize();
	info.psramFree = ESP.getFreePsram();
	info.flashSize = ESP.getFlashChipSize();
	info.sketchSize = ESP.getSketchSize();
	info.freeSketchSpace = ESP.getFreeSketchSpace();

#if defined(BOARD_HAS_PSRAM)
	info.psramLargestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
#else
	info.psramLargestBlock = 0;
#endif

	return info;
}

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
	SystemMemoryInfo info = get_system_memory_info();

	Serial.println("[SYS] Memory / flash info:");

	print_bytes_kb("  Heap total: ", info.heapTotal);
	print_bytes_kb("  Heap free : ", info.heapFree);
	print_bytes_kb("  Heap min  : ", info.heapMinFree);
	print_bytes_kb("  PSRAM size: ", info.psramTotal);
	print_bytes_kb("  PSRAM free: ", info.psramFree);

	Serial.print("  Flash size: ");
	Serial.print(info.flashSize);
	Serial.println(" bytes");

	Serial.print("  Sketch size: ");
	Serial.print(info.sketchSize);
	Serial.println(" bytes");

	Serial.print("  Free sketch space: ");
	Serial.print(info.freeSketchSpace);
	Serial.println(" bytes");

	Serial.print("  Largest 8-bit heap block: ");
	Serial.println(info.heapLargestBlock);

	Serial.print("  Largest PSRAM block: ");
	Serial.println(info.psramLargestBlock);
}
