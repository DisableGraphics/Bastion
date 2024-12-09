#include <kernel/drivers/disk/ahci.hpp>
#include <kernel/memory/page.hpp>
#include <stdio.h>

AHCI::AHCI(const PCI::PCIDevice &device) : DiskDriver(device){
	PCI &pci = PCI::get();
	for(int i = 0; i < 6; i++) {
		bars[i] = pci.getBAR(device.bus, device.device, device.function, i);

		printf("%p\n", bars[i]);
	}
	PagingManager::get().map_page(reinterpret_cast<void*>(bars[5]), reinterpret_cast<void*>(bars[5]), 
	CACHE_DISABLE | READ_WRITE | PRESENT);
	uint8_t *ptr = reinterpret_cast<uint8_t*>(bars[5]);

	printf("Val: %p\n", *ptr);
}