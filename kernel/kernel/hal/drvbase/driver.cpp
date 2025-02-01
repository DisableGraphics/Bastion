#include <kernel/hal/drvbase/driver.hpp>

size_t hal::Driver::get_irqline() {
	return irqline;
}

bool hal::Driver::is_exclusive_irq() {
	return exclusive_irq;
}