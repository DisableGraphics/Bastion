#pragma once
#include <stdint.h>
#include <kernel/hal/drvbase/irqc.hpp>
#ifdef __i386
#include "../arch/i386/defs/pic/pic.hpp"
#endif
/**
	\brief Programmable Interrupt Controller driver.

	Implemented as a singleton.
 */
class PIC : public hal::IRQController {
	public:
		PIC();
		void disable() override;
		void disable_irq(size_t irqline) override;
		void enable_irq(size_t irqline) override;
		void eoi(size_t irqline) override;
		void ack(size_t irqline) override;
		void register_driver(hal::Driver* driver, size_t irqline) override;
		
		size_t get_default_irq(hal::Device dev) override;
		size_t assign_irq(hal::Driver* device) override;
	private:
		/**
			\brief Get generic IRQ register
		 */
		uint16_t get_irq_reg(int ocw3);
		/**
			\brief Remap the PIC to offset1 and offset1.
			\param offset1 Offset for the master PIC.
			\param offset2 Offset for the slave PIC.
		 */
		void remap(int offset1, int offset2);
		/**
			\brief Initialise the PIC.
			Remaps the PIC to a more manageable position in the IDT
			and disables all lines.
		 */
		void init();

		/**
			\brief Get IRR (Interrupt Request Register)
		 */
		uint16_t get_irr();
		/**
			\brief Get ISR (Interrupt Status Register)
		 */
		uint16_t get_isr();
		/**
			\brief Get PIC mapping offset (0x20)
		 */
		uint8_t get_offset();
};