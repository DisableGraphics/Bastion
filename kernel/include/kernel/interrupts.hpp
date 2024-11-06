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

struct interrupt_frame;

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
		__attribute__((aligned(0x10)))
		idt_entry_t idt[IDT_MAX_DESCRIPTORS];
		__attribute__((aligned(0x4)))
		void * isr_table[32];
		bool vectors[32];
		idtr_t idtr;

		// Interrupt handlers
		__attribute__((interrupt))
		static void generic_exception_handler(interrupt_frame*);
		__attribute__((interrupt)) // 0
		static void division_by_zero_handler(interrupt_frame*);
		__attribute__((interrupt)) // 1
		static void debug_handler(interrupt_frame*);
		__attribute__((interrupt)) // 2
		static void nmi_handler(interrupt_frame*);
		__attribute__((interrupt)) // 3
		static void breakpoint_handler(interrupt_frame*);
		__attribute__((interrupt)) // 4
		static void overflow_handler(interrupt_frame*);
		__attribute__((interrupt)) // 5
		static void bound_range_exceeded_handler(interrupt_frame*);
		__attribute__((interrupt)) // 6
		static void invalid_opcode_handler(interrupt_frame*);
		__attribute__((interrupt)) // 7
		static void device_not_available_handler(interrupt_frame*);
		__attribute__((interrupt)) // 8
		static void double_fault_handler(interrupt_frame*);
		__attribute__((interrupt)) // 10
		static void invalid_tss_handler(interrupt_frame*, unsigned int);
		__attribute__((interrupt)) // 11
		static void segment_not_present_handler(interrupt_frame*, unsigned int);
		__attribute__((interrupt)) // 12
		static void stack_segment_fault_handler(interrupt_frame*, unsigned int);
		__attribute__((interrupt)) // 13
		static void general_protection_fault_handler(interrupt_frame*, unsigned int);
		__attribute__((interrupt)) // 14
		static void page_fault_handler(interrupt_frame*, unsigned int);
		__attribute__((interrupt)) // 16
		static void floating_point_exception_handler(interrupt_frame*);
		__attribute__((interrupt)) // 17
		static void alignment_check_handler(interrupt_frame*, unsigned int);
		__attribute__((interrupt)) // 18
		static void machine_check_handler(interrupt_frame*);
		__attribute__((interrupt)) // 19
		static void simd_floating_point_exception_handler(interrupt_frame*);
		__attribute__((interrupt)) // 20
		static void virtualization_exception_handler(interrupt_frame*);
		__attribute__((interrupt)) // 21
		static void control_protection_exception_handler(interrupt_frame*, unsigned int);
		__attribute__((interrupt)) // 28
		static void hypervisor_injection_exception_handler(interrupt_frame*);
		__attribute__((interrupt)) // 29
		static void vmm_communication_exception_handler(interrupt_frame*, unsigned int);
		__attribute__((interrupt)) // 30
		static void security_exception_handler(interrupt_frame*, unsigned int);
};

extern IDT idt;