#pragma once
#include <stdint.h>
#include <stdbool.h>
#ifdef __i386
#include "../arch/i386/defs/idt_entry.h"
#include "../arch/i386/defs/idtr.h"
#endif

#define IDT_MAX_DESCRIPTORS 256

extern "C" __attribute__((noreturn)) void exception_handler(void);
extern void* isr_stub_table[];

class IDT {
	public:
		void init();
		void set_descriptor(uint8_t vector, void* isr, uint8_t flags);
		idtr_t get_idtr();
	private:
		__attribute__((aligned(0x10)))
		idt_entry_t idt[IDT_MAX_DESCRIPTORS];
		bool vectors[32];
		idtr_t idtr;
};

extern IDT idt;