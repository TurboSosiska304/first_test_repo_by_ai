#pragma once

#include <stddef.h>

void init_system_memory();
void print_system_memory_info();

struct SystemMemoryInfo {
	bool psramAvailable;
	size_t heapTotal;
	size_t heapFree;
	size_t heapMinFree;
	size_t heapLargestBlock;
	size_t psramTotal;
	size_t psramFree;
	size_t psramLargestBlock;
	size_t flashSize;
	size_t sketchSize;
	size_t freeSketchSpace;
};

SystemMemoryInfo get_system_memory_info();
