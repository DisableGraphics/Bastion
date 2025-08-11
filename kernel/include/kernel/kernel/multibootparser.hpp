#pragma once
#include <stdint.h>
struct Multiboot2Info {
	// Pointers to the relevant tags or nullptr if missing
	const struct multiboot_tag_mmap* memory_map = nullptr;
	const struct multiboot_tag_module* modules = nullptr;
	uint32_t modules_count = 0;
	const struct multiboot_tag_string* cmdline = nullptr;
	const struct multiboot_tag_framebuffer* fb = nullptr;
	const void* last_module_address = nullptr;
};

Multiboot2Info parse_multiboot2_info(const uint8_t* mb_info_ptr);