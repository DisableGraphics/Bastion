#include "kernel/inlineasm.h"
#include <kernel/interrupts.hpp>
#include <kernel/serial.hpp>
#include <stdio.h>

IDT idt;
struct interrupt_frame;

extern "C" void exception_handler(void) {
	printf("Unknown error.\n");
}

void IDT::set_descriptor(uint8_t vector, void* isr, uint8_t flags) {
    idt_entry_t* descriptor = &idt[vector];

    descriptor->isr_low        = (uint32_t)isr & 0xFFFF;
    descriptor->kernel_cs      = 0x08; // this value can be whatever offset your kernel code selector is in your GDT
    descriptor->attributes     = flags;
    descriptor->isr_high       = (uint32_t)isr >> 16;
    descriptor->reserved       = 0;
}

void IDT::init() {
    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;

	fill_isr_table();

    for (uint32_t vector = 0; vector < 32; vector++) {
        set_descriptor(vector, isr_table[vector], 0x8E);
		//printf("%p ", isr_stub_table[vector]);
        vectors[vector] = true;
    }

	set_idtr(idtr);
    enable_interrupts();
}

void IDT::enable_interrupts() {
	__asm__ __volatile__("sti");
}

void IDT::disable_interrupts() {
	__asm__ __volatile__("cli");
}

void IDT::set_idtr(idtr_t idtr) {
	__asm__ __volatile__ ("lidt %0" : : "m"(idtr)); // load the new IDT
}

idtr_t IDT::get_idtr() {
	idtr_t ret;
	__asm__ __volatile__("sidt %0" : "=m"(ret));
	return ret;
}

void IDT::fill_isr_table() {
	isr_table[0] = (void*)&division_by_zero_handler;
	
	for(int i = 1; i < 32; i++) {
		isr_table[i] = (void*)&generic_interrupt_handler;
	}
}