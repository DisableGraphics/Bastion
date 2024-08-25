#pragma once
#include <stdint.h>
#pragma pack(1)
struct GDT{
	
};
#pragma pack()

void encodeGdtEntry(uint8_t *target, struct GDT source);