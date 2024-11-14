#pragma once
#include <stdint.h>
#ifdef __i386
#include "../arch/i386/defs/pic/pic.hpp"
#endif

class PIC {
	public:
		static PIC &get();
		void init();
		void remap(int offset1, int offset2);
		void disable();

		void IRQ_set_mask(uint8_t IRQ_line);
		void IRQ_clear_mask(uint8_t IRQ_line);

		void send_EOI(uint8_t irq);

		uint16_t get_irr();
		uint16_t get_isr();
	private:
		uint16_t get_irq_reg(int ocw3);
		PIC(){};

};