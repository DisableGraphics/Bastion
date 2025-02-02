#include <kernel/hal/drvbase/irqc.hpp>

void hal::IRQController::interrupt_handler(size_t irqline) {
	for(size_t i = 0; i < irq_lines[irqline].size(); i++) {
		irq_lines[irqline][i]->handle_interrupt();
	}
	eoi(irqline);
}