#pragma once
#include <stdint.h>
/**
	\brief GDT entry
 */
struct GDT_entry {
	uint32_t limit;
	uint32_t base;
	uint8_t access_byte;
	uint8_t flags;
};