#pragma once
#include <stdint.h>
#include <stdbool.h>

#define IDT_MAX_DESCRIPTORS 256

struct idt_entry_t {
	uint16_t    isr_low;      // The lower 16 bits of the ISR's address
	uint16_t    kernel_cs;    // The GDT segment selector that the CPU will load into CS before calling the ISR
	uint8_t     reserved;     // Set to zero
	uint8_t     attributes;   // Type and attributes; see the IDT page
	uint16_t    isr_high;     // The higher 16 bits of the ISR's address
} __attribute__((packed));

__attribute__((aligned(0x10))) 
static idt_entry_t idt[256];

typedef struct {
	uint16_t	limit;
	uint32_t	base;
} __attribute__((packed)) idtr_t;
extern idtr_t idtr;

extern "C" __attribute__((noreturn)) void exception_handler(void);
void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags);

static bool vectors[32];

extern void* isr_stub_table[];

void init_idt(void);