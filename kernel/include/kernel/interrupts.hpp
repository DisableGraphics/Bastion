#pragma once
#include <stdint.h>
#include <stdbool.h>
#ifdef __i386
#include "../arch/i386/defs/idt_entry.h"
#include "../arch/i386/defs/idtr.h"
#endif

#define IDT_MAX_DESCRIPTORS 256

extern "C" void exception_handler(void);
extern void* isr_stub_table[];

struct interrupt_frame {
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t sp;
    uint32_t ss;
} __attribute__((packed));

// Interrupt Descriptor Table
class IDT {
	public:
		void init();
		void set_descriptor(uint8_t vector, void* isr, uint8_t flags);
		idtr_t get_idtr();
		static void enable_interrupts();
		static void disable_interrupts();
	private:
		void set_idtr(idtr_t idtr);
		void fill_isr_table();

		__attribute__((interrupt))
		static void generic_interrupt_handler(interrupt_frame*);

		__attribute__((interrupt))
		static void division_by_zero_handler(interrupt_frame*);

		__attribute__((aligned(0x10)))
		idt_entry_t idt[IDT_MAX_DESCRIPTORS];
		__attribute__((aligned(0x4)))
		void * isr_table[IDT_MAX_DESCRIPTORS];
		bool vectors[32];
		idtr_t idtr;
};

extern IDT idt;