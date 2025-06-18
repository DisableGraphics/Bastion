#include "distorm/print_disassemble.h"
#include "kernel/assembly/inlineasm.h"
#include <kernel/drivers/interrupts.hpp>
#include <stdio.h>
#include <kernel/kernel/log.hpp>
#include <kernel/hal/managers/irqcmanager.hpp>
#include "../defs/regs/regs.h"
#include <kernel/scheduler/scheduler.hpp>
#include <kernel/kernel/stable.h>

struct interrupt_frame
{
    uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
};

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
	IDT::disable_interrupts();
	halt();
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
	clear();
	log(ERROR, "General Protection Fault:\n - External: %d\n - Tbl: %s\n - Index: %p\n", 
		ecode & 0x1, 
		table((ecode & 0x6) >> 1),
		(ecode & 0xFFF8) >> 3);
	IDT::disable_interrupts();
	halt();
}

void IDT::page_fault_handler(interrupt_frame* ifr, unsigned int ecode) {
	regs r;
	COPY_REGS(r);
	uint32_t eip = ifr->eip;

	uint32_t faulting_address = read_cr2();
	const char * prot_type = (ecode & 1) ? "page protection" : "not present";
	const char * rw = (ecode & 0x2) ? "Write" : "Read";
	const char * user = (ecode & 0x4) ? "User" : "Kernel";
	const char * instruction = (ecode & 0x10) ? "Fetch" : "No fetch"; 
	log(ERROR,"Page fault: %s %s %s %s", prot_type, rw, user, instruction);
	log(ERROR, "%s interrupt", hal::IRQControllerManager::get().in_interrupt() ? "In" : "Not in");
	log(ERROR, "IRQLINE (if in interrupt, else -1): %d", hal::IRQControllerManager::get().get_current_handled_irqline());
	log(ERROR, "Tried to access %p, which is not a mapped address", faulting_address);
	log(ERROR, "Instruction pointer: %p", eip);

	Task* current_task;

	if((current_task = Scheduler::get().get_current_task())) {
		log(ERROR, "Current task: %d", current_task->id);
	}

	log(ERROR, "EAX: %p", r.eax);
	log(ERROR, "EBX: %p", r.ebx);
	log(ERROR, "ECX: %p", r.ecx);
	log(ERROR, "EDX: %p", r.edx);
	log(ERROR, "EDI: %p", r.edi);
	log(ERROR, "ESI: %p", r.esi);

	print_disassemble(reinterpret_cast<void*>(eip-32), 48, reinterpret_cast<void*>(eip));

	log(ERROR, "Crashed at: %s", find_function_name(eip));

	IDT::disable_interrupts();
	halt();
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