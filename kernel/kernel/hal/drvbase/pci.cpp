#include <kernel/hal/drvbase/pci.hpp>
#include <kernel/kernel/log.hpp>
#include <kernel/hal/managers/irqcmanager.hpp>

#ifdef __i386__
#include "../arch/i386/defs/pci/offsets/header_0.hpp"
#endif

hal::PCIDriver::PCIDriver(const PCISubsystemManager::PCIDevice &device) : device(device) {

}

void hal::PCIDriver::basic_setup() {
	irqline = hal::PCISubsystemManager::get().readConfigWord(device.bus, device.device, device.function, INTERRUPT_LINE)
	& 0xFF;

	if(irqline == 0xFF) {
		log(ERROR, "PCI device doesn't have default interrupt line");
	} else {
		log(INFO, "IRQ assigned for pci device: %d", irqline);
		hal::IRQControllerManager::get().register_driver(this, irqline);
		hal::IRQControllerManager::get().enable_irq(irqline);	
	}
}