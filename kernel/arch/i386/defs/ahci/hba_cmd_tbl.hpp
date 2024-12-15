#pragma once
#include <stdint.h>
#include "hba_prdt_entry.hpp"

typedef struct tagHBA_CMD_TBL
{
	// 0x00
	uint8_t  cfis[64];	// Command FIS

	// 0x40
	uint8_t  acmd[16];	// ATAPI command, 12 or 16 bytes

	// 0x50
	uint8_t  rsv[48];	// Reserved

	// 0x80
	HBA_PRDT_ENTRY	prdt_entry[1];	// Physical region descriptor table entries, 0 ~ 65535
} HBA_CMD_TBL;
