#include "../arch/i386/defs/idt/idt_entry.h"
#include <kernel/drivers/interrupts.hpp>
#include <kernel/drivers/serial.hpp>
#include <stdio.h>

struct interrupt_frame;

extern "C" void exception_handler(void) {
	printf("Unknown error.\n");
}

IDT& IDT::get() {
	static IDT instance;
	return instance;
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

void IDT::set_handler(uint8_t vector, void (*fn)(interrupt_frame*)) {
	isr_table[vector] = reinterpret_cast<void*>(fn);
	set_descriptor(vector, isr_table[vector], 0x8E);
}

void IDT::set_handler(uint8_t vector, void (*fn)(interrupt_frame*, uint32_t ecode)) {
	isr_table[vector] = reinterpret_cast<void*>(fn);
	set_descriptor(vector, isr_table[vector], 0x8E);
}

void IDT::fill_isr_table() {
	isr_table[0] = (void*)&division_by_zero_handler;
	isr_table[1] = (void*)&debug_handler;
	isr_table[2] = (void*)&nmi_handler;
	isr_table[3] = (void*)&breakpoint_handler;
	isr_table[4] = (void*)&overflow_handler;
	isr_table[5] = (void*)&bound_range_exceeded_handler;
	isr_table[6] = (void*)&invalid_opcode_handler;
	isr_table[7] = (void*)&device_not_available_handler;
	isr_table[8] = (void*)&double_fault_handler;
	isr_table[9] = (void*)&generic_exception_handler; // Legacy. Won't be implemented
	isr_table[10] = (void*)&invalid_tss_handler;
	isr_table[11] = (void*)&segment_not_present_handler;
	isr_table[12] = (void*)&stack_segment_fault_handler;
	isr_table[13] = (void*)&general_protection_fault_handler;
	isr_table[14] = (void*)&page_fault_handler;
	isr_table[15] = (void*)&generic_exception_handler; // Reserved
	isr_table[16] = (void*)&floating_point_exception_handler;
	isr_table[17] = (void*)&alignment_check_handler;
	isr_table[18] = (void*)&machine_check_handler;
	isr_table[19] = (void*)&simd_floating_point_exception_handler;
	isr_table[20] = (void*)&virtualization_exception_handler;
	isr_table[21] = (void*)&control_protection_exception_handler;
	isr_table[22] = (void*)&generic_exception_handler; // Reserved
	isr_table[23] = (void*)&generic_exception_handler; // Reserved
	isr_table[24] = (void*)&generic_exception_handler; // Reserved
	isr_table[25] = (void*)&generic_exception_handler; // Reserved
	isr_table[26] = (void*)&generic_exception_handler; // Reserved
	isr_table[27] = (void*)&generic_exception_handler; // Reserved
	isr_table[28] = (void*)&hypervisor_injection_exception_handler;
	isr_table[29] = (void*)&vmm_communication_exception_handler;
	isr_table[30] = (void*)&security_exception_handler;
	isr_table[31] = (void*)&generic_exception_handler; // Reserved
}

idt_entry_t *IDT::get_idt() {
	return idt;
}

void **IDT::get_isr_table() {
	return isr_table;
}