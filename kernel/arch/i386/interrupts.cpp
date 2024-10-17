#include <kernel/interrupts.hpp>
#include <stdio.h>
#include <kernel/serial.h>

idtr_t idtr;
__attribute__((aligned(0x10))) 
idt_entry_t idt[256];

__attribute__((noreturn))
extern "C" void exception_handler(void) {
	printf("S-egg-mentation fault (core egged)\n");
    __asm__ __volatile__ ("cli; hlt"); // Completely hangs the computer
}

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags) {
    idt_entry_t* descriptor = &idt[vector];

    descriptor->isr_low        = (uint32_t)isr & 0xFFFF;
    descriptor->kernel_cs      = 0x08; // this value can be whatever offset your kernel code selector is in your GDT
    descriptor->attributes     = flags;
    descriptor->isr_high       = (uint32_t)isr >> 16;
    descriptor->reserved       = 0;
}

void init_idt() {
    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;

    for (uint32_t vector = 0; vector < 32; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        vectors[vector] = true;
    }

    __asm__ volatile ("lidt %0" : : "m"(idtr)); // load the new IDT
    __asm__ volatile ("sti"); // set the interrupt flag
}

uint64_t get_idtr() {
	idtr_t ret;
	__asm__ __volatile__("sidt %0" : "=m"(ret));
	uint64_t ret2 = ret.limit;
	ret2 <<= 32;
	ret2 |= ret.base;
	return ret2;
}