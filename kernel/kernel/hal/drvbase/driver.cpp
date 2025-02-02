#include <kernel/hal/drvbase/driver.hpp>
#include <kernel/hal/managers/irqcmanager.hpp>
#include <kernel/kernel/log.hpp>

size_t hal::Driver::get_irqline() {
	return irqline;
}

bool hal::Driver::is_exclusive_irq() {
	return exclusive_irq;
}

hal::Device hal::Driver::get_device_type() {
	return device_type;
}

void hal::Driver::basic_setup(hal::Device device) {
	irqline = hal::IRQControllerManager::get().get_default_irq(device);
	if(irqline == -1)
		irqline = hal::IRQControllerManager::get().assign_irq(this);

	log(INFO, "IRQ assigned: %d", irqline);
	hal::IRQControllerManager::get().register_driver(this, irqline);
	hal::IRQControllerManager::get().enable_irq(irqline);
}