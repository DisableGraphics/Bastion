#include <kernel/drivers/interrupts.hpp>
#include <stdio.h>

void IDT::generic_exception_handler(interrupt_frame*) {
	printf("Unknown error: This shouldn't happen. WTF have I done?\n");
}

void IDT::division_by_zero_handler(interrupt_frame*) {
	printf("Division by zero\n");
}

void IDT::debug_handler(interrupt_frame*) {
	printf("DEBUG\n");
}

void IDT::nmi_handler(interrupt_frame*) {
	printf("Non Maskable Interrupt\n");
}

void IDT::breakpoint_handler(interrupt_frame*) {
	printf("Breakpoint\n");
}

void IDT::overflow_handler(interrupt_frame*) {
	printf("Overfloeeee\n");
}

void IDT::bound_range_exceeded_handler(interrupt_frame*) {
	printf("Bound Range Exceeded\n");
}

void IDT::invalid_opcode_handler(interrupt_frame*) {
	printf("Invalid OPCODE\n");
}

void IDT::device_not_available_handler(interrupt_frame*) {
	printf("Device NOT available\n");
}

void IDT::double_fault_handler(interrupt_frame*) {
	printf("Double fault\n");
}

void IDT::invalid_tss_handler(interrupt_frame*, unsigned int) {
	printf("Invalid TSS\n");
}

void IDT::segment_not_present_handler(interrupt_frame*, unsigned int) {
	printf("Segment not present\n");
}

void IDT::stack_segment_fault_handler(interrupt_frame*, unsigned int) {
	printf("Stack segment fault\n");
}

[[gnu::no_caller_saved_registers]]
const char * table(int code) {
	switch(code) {
		case 0:
			return "GDT";
		case 1:
			return "IDT";
		case 2:
			return "LDT";
		case 3:
			return "IDT";
		default:
			return "Unknown";
	}
}

void IDT::general_protection_fault_handler(interrupt_frame*, unsigned int ecode) {
	printf("General Protection Fault:\n - External: %d\n - Tbl: %s\n - Index: %p\n", 
		ecode & 0x1, 
		table((ecode & 0x6) >> 1),
		(ecode & 0xFFF8) >> 3);
}

void IDT::page_fault_handler(interrupt_frame*, unsigned int ecode) {
	const char * prot_type = (ecode & 1) ? "page protection" : "not present";
	const char * rw = (ecode & 0x2) ? "Write" : "Read";
	const char * user = (ecode & 0x4) ? "User" : "Kernel";
	const char * instruction = (ecode & 0x10) ? "Fetch" : "No fetch"; 
	printf("Page fault: %s %s %s %s\n", prot_type, rw, user, instruction);
}

void IDT::floating_point_exception_handler(interrupt_frame*) {
	printf("Floating point exception\n");
}

void IDT::alignment_check_handler(interrupt_frame *, unsigned int) {
	printf("Alignment check\n");
}

void IDT::machine_check_handler(interrupt_frame *) {
	printf("Machine check\n");
}

void IDT::simd_floating_point_exception_handler(interrupt_frame*) {
	printf("SIMD floating point exception\n");
}

void IDT::virtualization_exception_handler(interrupt_frame*) {
	printf("VIRT exception\n");
}

void IDT::control_protection_exception_handler(interrupt_frame*, unsigned int) {
	printf("Control Protection Exception\n");
}

void IDT::hypervisor_injection_exception_handler(interrupt_frame*) {
	printf("Hypervisor Injection Exception\n");
}

void IDT::vmm_communication_exception_handler(interrupt_frame*, unsigned int) {
	printf("VMM Communication Exception\n");
}

void IDT::security_exception_handler(interrupt_frame*, unsigned int) {
	printf("Security Exception\n");
}