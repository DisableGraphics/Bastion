#include <kernel/drivers/disk/ahci.hpp>
#include <stdio.h>

AHCI::AHCI(const PCI::PCIDevice &device) : DiskDriver(device){
	PCI &pci = PCI::get();
	for(int i = 0; i < 6; i++) {
		bars[i] = pci.getBAR(device.bus, device.device, device.function, i);
		printf("%p\n", bars[i]);
	}
}