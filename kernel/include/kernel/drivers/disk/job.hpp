#pragma once
#include <stdint.h>

struct DiskJob {
	enum state {
		ERROR,
		FINISHED,
		WAITING
	} state;
	uint8_t * buffer;
	
};