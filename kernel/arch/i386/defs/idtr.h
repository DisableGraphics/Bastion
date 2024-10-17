#pragma once
#include <stdint.h>

struct idtr_t {
	uint16_t	limit;
	uint32_t	base;
} __attribute__((packed));