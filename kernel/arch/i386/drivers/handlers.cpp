#include "distorm/print_disassemble.h"
#include "kernel/assembly/inlineasm.h"
#include <kernel/drivers/interrupts.hpp>
#include <stdio.h>
#include <kernel/kernel/log.hpp>
#include <kernel/hal/managers/irqcmanager.hpp>
#include "../defs/regs/regs.h"
#include <kernel/scheduler/scheduler.hpp>
#include <kernel/kernel/stable.h>
#include <kernel/memory/mmanager.hpp>

struct interrupt_frame
{
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
};

void IDT::generic_exception_handler(interrupt_frame*) {
	log(ERROR, "Unknown error: This shouldn't happen. WTF have I done?\n");
}

void IDT::division_by_zero_handler(interrupt_frame*) {
	log(ERROR, "Division by zero\n");
}

void IDT::debug_handler(interrupt_frame* frame) {
	regs r;
	COPY_REGS(r);

	// Read debug status and control registers
	size_t dr6, dr7;
	asm volatile ("movl %%dr6, %0" : "=r"(dr6));
	asm volatile ("movl %%dr7, %0" : "=r"(dr7));

	log(ERROR, "DEBUG EXCEPTION at EIP=0x%p", frame->eip);
	log(ERROR, "Registers: EAX=0x%p EBX=0x%p ECX=0x%p EDX=0x%p", r.eax, r.ebx, r.ecx, r.edx);
	log(ERROR, "DR6=0x%llx DR7=0x%llx", dr6, dr7);

	// Check which breakpoint triggered (bits 0-3)
	for (int i = 0; i < 4; ++i) {
		if (dr6 & (1 << i)) {
			log(ERROR, "Breakpoint %d triggered", i);
		}
	}

	// Clear the sticky bits in DR6 (bits 0-3) to avoid retriggering
	asm volatile ("movl %0, %%dr6" :: "r"(0ULL));

	// Optional: disable interrupts to avoid nested exceptions if you want to halt
	IDT::disable_interrupts();

	// You might want to halt or return depending on your debug needs
	halt();
}

void IDT::nmi_handler(interrupt_frame*) {
	log(ERROR, "Non Maskable Interrupt\n");
}

void IDT::breakpoint_handler(interrupt_frame*) {
	log(ERROR, "Breakpoint\n");
}

void IDT::overflow_handler(interrupt_frame*) {
	log(ERROR, "Overfloeeee\n");
}

void IDT::bound_range_exceeded_handler(interrupt_frame*) {
	log(ERROR, "Bound Range Exceeded\n");
}

void IDT::invalid_opcode_handler(interrupt_frame*) {
	log(ERROR, "Invalid OPCODE\n");
}

void IDT::device_not_available_handler(interrupt_frame*) {
	log(ERROR, "Device NOT available\n");
}

void IDT::double_fault_handler(interrupt_frame* ir) {
	auto eip = ir->eip;

	log(ERROR, "Double fault\n");
	log(ERROR, "Crashed at: %s (%p)", find_function_name(eip), eip);

	IDT::disable_interrupts();
	halt();
}

void IDT::invalid_tss_handler(interrupt_frame*, unsigned int) {
	log(ERROR, "Invalid TSS\n");
}

void IDT::segment_not_present_handler(interrupt_frame*, unsigned int) {
	log(ERROR, "Segment not present\n");
}

void IDT::stack_segment_fault_handler(interrupt_frame*, unsigned int) {
	log(ERROR, "Stack segment fault\n");
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
	uintptr_t esp;
	asm volatile("mov %%esp, %0" : "=r"(esp));
	regs r;
	COPY_REGS(r);
	uint32_t eip = ifr->eip;

	uint32_t faulting_address = read_cr2();
	const char * prot_type = (ecode & 1) ? "page protection" : "not present";
	const char * rw = (ecode & 0x2) ? "Write" : "Read";
	const char * user = (ecode & 0x4) ? "User" : "Kernel";
	const char * instruction = (ecode & 0x10) ? "Fetch" : "No fetch"; 
	log(ERROR,"Page fault: %s %s %s %s", prot_type, rw, user, instruction);
	
	log(ERROR, "Tried to access %p, which is not a mapped address", faulting_address);
	log(ERROR, "Instruction pointer: %p", eip);
	log(ERROR, "CS: %p", ifr->cs);
	log(ERROR, "Eflags: %p", ifr->eflags);

	log(ERROR, "EAX: %p", r.eax);
	log(ERROR, "EBX: %p", r.ebx);
	log(ERROR, "ECX: %p", r.ecx);
	log(ERROR, "EDX: %p", r.edx);
	log(ERROR, "EDI: %p", r.edi);
	log(ERROR, "ESI: %p", r.esi);
	log(ERROR, "ESP at interrupt time: %p", esp);

	const char* fname = find_function_name(eip);
	log(ERROR, "Crashed at: %s", fname);

	MemoryManager::get().dump_recent_allocs();

	log(ERROR, "%s interrupt", hal::IRQControllerManager::get().in_interrupt() ? "In" : "Not in");
	log(ERROR, "IRQLINE (if in interrupt, else -1): %d", hal::IRQControllerManager::get().get_current_handled_irqline());

	/*Task* current_task;

	if((current_task = Scheduler::get().get_current_task())) {
		log(ERROR, "Current task: %d", current_task->id);
	}*/

	//print_disassemble(reinterpret_cast<void*>(eip-32), 48, reinterpret_cast<void*>(eip));	

	IDT::disable_interrupts();
	halt();
}

void IDT::floating_point_exception_handler(interrupt_frame*) {
	log(ERROR, "Floating point exception\n");
}

void IDT::alignment_check_handler(interrupt_frame *, unsigned int) {
	log(ERROR, "Alignment check\n");
}

void IDT::machine_check_handler(interrupt_frame *) {
	log(ERROR, "Machine check\n");
}

void IDT::simd_floating_point_exception_handler(interrupt_frame*) {
	log(ERROR, "SIMD floating point exception\n");
}

void IDT::virtualization_exception_handler(interrupt_frame*) {
	log(ERROR, "VIRT exception\n");
}

void IDT::control_protection_exception_handler(interrupt_frame*, unsigned int) {
	log(ERROR, "Control Protection Exception\n");
}

void IDT::hypervisor_injection_exception_handler(interrupt_frame*) {
	log(ERROR, "Hypervisor Injection Exception\n");
}

void IDT::vmm_communication_exception_handler(interrupt_frame*, unsigned int) {
	log(ERROR, "VMM Communication Exception\n");
}

void IDT::security_exception_handler(interrupt_frame*, unsigned int) {
	log(ERROR, "Security Exception\n");
}