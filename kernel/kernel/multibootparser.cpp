#include "kernel/memory/page.hpp"
#include <kernel/kernel/multibootparser.hpp>
#include <multiboot/multiboot2.h>

inline uint32_t align8(uint32_t size) {
	return (size + 7) & ~7;
}

Multiboot2Info parse_multiboot2_info(const uint8_t* mb_info_ptr) {
	Multiboot2Info info{};

	uint32_t total_size = *(const uint32_t*)mb_info_ptr;
	const uint8_t* tag_ptr = mb_info_ptr + 8; // Skip total_size and reserved
	const uint8_t* end_ptr = mb_info_ptr + total_size;

	while (tag_ptr < end_ptr) {
		auto tag = reinterpret_cast<const multiboot_tag*>(tag_ptr);
		log(INFO, "%p", tag);
		if (tag->type == MULTIBOOT_TAG_TYPE_END) {
			break;
		}

		switch (tag->type) {
			case MULTIBOOT_TAG_TYPE_MODULE: {
				log(INFO, "modules at %p", tag);
				// We could handle multiple modules here if needed
				info.modules = reinterpret_cast<const multiboot_tag_module*>(tag);
				
				// Count modules? The spec doesn't say how many modules directly,
				// but the bootloader might provide count or you count them yourself.
				// For simplicity, let's just set count = 1 here.
				info.modules_count = 1;
				if((uint32_t)info.last_module_address < (info.modules->mod_end + HIGHER_HALF_OFFSET)) {
					info.last_module_address = reinterpret_cast<void*>(info.modules->mod_end + HIGHER_HALF_OFFSET);
				}
				break;
			}
			case MULTIBOOT_TAG_TYPE_MMAP:
				info.memory_map = reinterpret_cast<const multiboot_tag_mmap*>(tag);
				if((uint32_t)info.last_module_address < ((size_t)info.memory_map + info.memory_map->size)) {
					info.last_module_address = reinterpret_cast<void*>((size_t)info.memory_map + info.memory_map->size);
				}
				break;
			case MULTIBOOT_TAG_TYPE_CMDLINE:
				info.cmdline = reinterpret_cast<const multiboot_tag_string*>(tag);
				if((uint32_t)info.last_module_address < ((size_t)info.cmdline + info.cmdline->size)) {
					info.last_module_address = reinterpret_cast<void*>((size_t)info.cmdline + info.cmdline->size);
				}
				break;
			case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
				info.fb = reinterpret_cast<const multiboot_tag_framebuffer*>(tag);
				if((uint32_t)info.fb < ((size_t)info.fb + info.fb->common.size)) {
					info.last_module_address = reinterpret_cast<void*>((size_t)info.fb + info.fb->common.size);
				}
				break;
			default:
				// ignore other tags for now
				log(INFO, "Tag type: %d", tag->type);
				break;
		}
		log(INFO, "%d", tag->size);
		tag_ptr += align8(tag->size);
	}

	return info;
}
