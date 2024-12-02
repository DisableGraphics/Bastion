#pragma once
#include <stdint.h>
#ifdef __i386
#include "../arch/i386/defs/pic/pic.hpp"
#endif
/**
	\brief Programmable Interrupt Controller driver.

	Implemented as a singleton.
 */
class PIC {
	public:
		/**
			\brief Get singleton instance
		 */
		static PIC &get();
		/**
			\brief Initialise the PIC.
			Remap the PIC to a more manageable position in the IDT
			and disable all lines. 
		 */
		void init();
		/**
			\brief Remap the PIC to offset1 and offset1.
			\param offset1 Offset for the master PIC.
			\param offset2 Offset for the slave PIC.
		 */
		void remap(int offset1, int offset2);
		/**
			\brief Disable all Interrupt Request lines.
		 */
		void disable_irqs();
		/**
			\brief Set mask for IRQ_line.
			This means IRQ_line won't send IRQs.
		 */
		void IRQ_set_mask(uint8_t IRQ_line);
		/**
			\brief Clear mask for IRQ_line.
			This means IRQ_line will send IRQs.
		 */
		void IRQ_clear_mask(uint8_t IRQ_line);
		/**
			\brief Send End Of Interrupt byte (EOI for short).
			if irq >= 8 it will send also EOI to the slave PIC.
		 */
		void send_EOI(uint8_t irq);
		/**
			\brief Get IRR (Interrupt Request Register)
		 */
		uint16_t get_irr();
		/**
			\brief Get ISR (Interrupt Status Register)
		 */
		uint16_t get_isr();
	private:
		/**
			\brief Get generic IRQ register
		 */
		uint16_t get_irq_reg(int ocw3);
		PIC(){};

};