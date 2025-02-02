#include <kernel/hal/managers/irqcmanager.hpp>

hal::IRQControllerManager::IRQControllerManager() {
	isr_setup();
}

hal::IRQControllerManager& hal::IRQControllerManager::get() {
	static IRQControllerManager instance;
	return instance;
}

void hal::IRQControllerManager::register_controller(hal::IRQController* controller) {
	controllers.push_back(controller);
}

void hal::IRQControllerManager::register_driver(hal::Driver* device, size_t irqline) {
	auto controller = find_controller(irqline);
	if(controller) controller->register_driver(device, irqline);
}

void hal::IRQControllerManager::enable_irq(size_t irqline) {
	auto controller = find_controller(irqline);
	if(controller) controller->enable_irq(irqline);
}

void hal::IRQControllerManager::disable_irq(size_t irqline) {
	auto controller = find_controller(irqline);
	if(controller) controller->disable_irq(irqline);
}

void hal::IRQControllerManager::set_irq_for_controller(hal::IRQController* controller, size_t irqline) {
	if(irqline < MAX_IRQS)
		irq_lines[irqline] = controller;
}

hal::IRQController* hal::IRQControllerManager::find_controller(size_t irqline) {
	if(irqline < MAX_IRQS)
		return irq_lines[irqline];
	return nullptr;
}

void hal::IRQControllerManager::eoi(size_t irqline) {
	auto controller = find_controller(irqline);
	if(controller) controller->eoi(irqline);
}

size_t hal::IRQControllerManager::get_default_irq(hal::Device dev) {
	if(controllers.size() == 0)
		return -1;
	return controllers[0]->get_default_irq(dev);
}

size_t hal::IRQControllerManager::assign_irq(hal::Driver* device) {
	if(controllers.size() == 0)
		return -1;
	/// TODO: Improve this for APIC-like systems
	return controllers[0]->assign_irq(device);
}