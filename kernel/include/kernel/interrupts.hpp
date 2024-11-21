#pragma once
#include <stdint.h>
#include <stdbool.h>
#ifdef __i386
#include "../arch/i386/defs/idt_entry.h"
#include "../arch/i386/defs/idtr.h"
#endif

#define IDT_MAX_DESCRIPTORS 256

struct interrupt_frame;

/**
	\brief Interrupt Descriptor Table
	Implemented as a singleton
*/
class IDT {
	public:
		/**
			\brief get singleton instance
		 */
		static IDT &get();
		/**
			\brief Initialise the IDT
		 */
		void init();
		/**
			\brief Set Interrupt Service Routine descriptor
			\param vector Number of interrupt
			\param isr Interrupt Service Routine (address of)
			\param flags Flags of the ISR
		 */
		void set_descriptor(uint8_t vector, void* isr, uint8_t flags);
		/**
			\brief Get Interrupt Descriptor Table Register
			\return active IDTR 
		*/
		idtr_t get_idtr();
		/**
			\brief get active interrupt descriptor table
		 */
		idt_entry_t *get_idt();
		/**
			\brief get interrupt service routine table
		 */
		void **get_isr_table();

		/**
			\brief Set interrupt service routine
			\param vector Interrupt number
			\param fn interrupt function
		 */
		void set_handler(uint8_t vector, void (*fn)(interrupt_frame*));
		void set_handler(uint8_t vector, void (*fn)(interrupt_frame*, uint32_t ecode));

		/**
			\brief Enable interrupts
		 */
		[[gnu::no_caller_saved_registers]]
		static void enable_interrupts();
		/**
			\brief Disable interrupts
		 */
		[[gnu::no_caller_saved_registers]]
		static void disable_interrupts();
	private:
		/**
			\brief sets the interrupt descriptor table register.
			\param idtr IDTR
			Tells the processor where the IDT is.
			Doesn't enable interrupts
		 */
		void set_idtr(idtr_t idtr);
		/**
			Fills the isr table with all exception handlers
		 */
		void fill_isr_table();
		/**
			\brief Interrupt Descriptor Table
		 */
		[[gnu::aligned(0x10)]]
		idt_entry_t idt[IDT_MAX_DESCRIPTORS];
		/**
			\brief ISR temporary table.
			The trick here is that it is aligned to a 4-byte boundary
			and so the processor doesn't have a brain fart when trying to
			call the interrupt handler
		 */
		[[gnu::aligned(0x4)]]
		void * isr_table[IDT_MAX_DESCRIPTORS];
		/**
			\brief IDT register
			Tells the processor where the IDT is
		 */
		idtr_t idtr;

		/**
			\brief Default interrupt/exception handler handler
		 */
		[[gnu::interrupt]]
		static void generic_exception_handler(interrupt_frame*);
		/**
			\brief Division by zero handler
		 */
		[[gnu::interrupt]]
		static void division_by_zero_handler(interrupt_frame*);
		/**
			\brief Debug exception handler
		 */
		[[gnu::interrupt]] // 1
		static void debug_handler(interrupt_frame*);
		/**
			\brief Non Maskable Interrupt handler
		 */
		[[gnu::interrupt]] // 2
		static void nmi_handler(interrupt_frame*);
		/**
			\brief Breakpoint interrupt handler
		 */
		[[gnu::interrupt]] // 3
		static void breakpoint_handler(interrupt_frame*);
		/**
			\brief Overflow exception handler
		 */
		[[gnu::interrupt]] // 4
		static void overflow_handler(interrupt_frame*);
		/**
			\brief Bound Range Exceeded exception handler
		 */
		[[gnu::interrupt]] // 5
		static void bound_range_exceeded_handler(interrupt_frame*);
		/**
			\brief Invalid Opcode Exception handler
		 */
		[[gnu::interrupt]] // 6
		static void invalid_opcode_handler(interrupt_frame*);
		/**
			\brief Device not available exception handler
		 */
		[[gnu::interrupt]] // 7
		static void device_not_available_handler(interrupt_frame*);
		/**
			\brief Double fault exception handler
		 */
		[[gnu::interrupt]] // 8
		static void double_fault_handler(interrupt_frame*);
		/**
			\brief Invalid TSS exception handler
		 */
		[[gnu::interrupt]] // 10
		static void invalid_tss_handler(interrupt_frame*, unsigned int);
		/**
			\brief Segment not present exception handler
		 */
		[[gnu::interrupt]] // 11
		static void segment_not_present_handler(interrupt_frame*, unsigned int);
		/**
			\brief Stack segment fault exception handler
		 */
		[[gnu::interrupt]] // 12
		static void stack_segment_fault_handler(interrupt_frame*, unsigned int);
		/**
			\brief General Protection fault exception handler
		 */
		[[gnu::interrupt]] // 13
		static void general_protection_fault_handler(interrupt_frame*, unsigned int);
		/**
			\brief Page fault exception handler
		 */
		[[gnu::interrupt]] // 14
		static void page_fault_handler(interrupt_frame*, unsigned int);
		/**
			\brief Floating point exception handler
		 */
		[[gnu::interrupt]] // 16
		static void floating_point_exception_handler(interrupt_frame*);
		/**
			\brief Alignment check exception handler
		 */
		[[gnu::interrupt]] // 17
		static void alignment_check_handler(interrupt_frame*, unsigned int);
		/**
			\brief Machine check exception handler
		 */
		[[gnu::interrupt]] // 18
		static void machine_check_handler(interrupt_frame*);
		/**
			\brief SIMD floating point exception handler
		 */
		[[gnu::interrupt]] // 19
		static void simd_floating_point_exception_handler(interrupt_frame*);
		/**
			\brief Virtualization exception handler
		 */
		[[gnu::interrupt]] // 20
		static void virtualization_exception_handler(interrupt_frame*);
		/**
			\brief Control protection exception handler
		 */
		[[gnu::interrupt]] // 21
		static void control_protection_exception_handler(interrupt_frame*, unsigned int);
		/**
			\brief Hypervisor injection exception handler
		 */
		[[gnu::interrupt]] // 28
		static void hypervisor_injection_exception_handler(interrupt_frame*);
		/**
			\brief VMM Communication Exception Handler
		 */
		[[gnu::interrupt]] // 29
		static void vmm_communication_exception_handler(interrupt_frame*, unsigned int);
		/**
			\brief Security exception handler
		 */
		[[gnu::interrupt]] // 30
		static void security_exception_handler(interrupt_frame*, unsigned int);

		IDT(){}
};