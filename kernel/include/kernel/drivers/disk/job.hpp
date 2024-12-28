#pragma once
#include <stdint.h>

struct DiskJob {
	enum state {
		ERROR,
		FINISHED,
		WAITING
	} state;

	DiskJob(uint8_t* buffer, uint64_t lba, uint32_t sector_count, bool write);

	uint8_t * buffer;
	uint64_t lba;
	uint32_t sector_count;
	bool write;
};