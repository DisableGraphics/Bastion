#pragma once
#include <stdint.h>

struct Partition {
	uint32_t id;
	uint64_t start_lba;
	uint64_t size;
	uint8_t type;
};