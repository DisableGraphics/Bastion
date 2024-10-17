#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t gdt[];

struct GDT {
	uint32_t limit;
	uint32_t base;
	uint8_t access_byte;
	uint8_t flags;
};
void init_gdt();
void encodeGdtEntry(uint8_t *target, struct GDT source);

#pragma pack(1)
struct GDTR
{
    uint16_t limit;
    uint32_t base;
};
#pragma pack()

extern uint8_t tss[0x6C];

#ifdef __cplusplus
}
#endif