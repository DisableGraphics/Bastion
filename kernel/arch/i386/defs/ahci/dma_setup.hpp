#pragma once
#include <stdint.h>
/**
	\brief DMA setup for a FIS in AHCI
 */
typedef struct tagFIS_DMA_SETUP
{
	// DWORD 0
	uint8_t  fis_type;	// FIS_TYPE_DMA_SETUP

	uint8_t  pmport:4;	// Port multiplier
	uint8_t  rsv0:1;		// Reserved
	uint8_t  d:1;		// Data transfer direction, 1 - device to host
	uint8_t  i:1;		// Interrupt bit
	uint8_t  a:1;            // Auto-activate. Specifies if DMA Activate FIS is needed

	uint8_t  rsved[2];       // Reserved

//DWORD 1&2

	uint64_t DMAbufferID;    // DMA Buffer Identifier. Used to Identify DMA buffer in host memory.
								// SATA Spec says host specific and not in Spec. Trying AHCI spec might work.

	//DWORD 3
	uint32_t rsvd;           //More reserved

	//DWORD 4
	uint32_t DMAbufOffset;   //Byte offset into buffer. First 2 bits must be 0

	//DWORD 5
	uint32_t TransferCount;  //Number of bytes to transfer. Bit 0 must be 0

	//DWORD 6
	uint32_t resvd;          //Reserved
        
} FIS_DMA_SETUP;
