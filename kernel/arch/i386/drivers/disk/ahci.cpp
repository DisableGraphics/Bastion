#include <kernel/drivers/disk/ahci.hpp>
#include <kernel/memory/page.hpp>
#include <stdio.h>
#include <kernel/memory/mmanager.hpp>

#include "../../defs/ahci/hba_mem.hpp"

AHCI::AHCI(const PCI::PCIDevice &device) : DiskDriver(device){
	PCI &pci = PCI::get();
	PagingManager& pm = PagingManager::get();
	for(int i = 0; i < 6; i++) {
		bars[i] = pci.getBAR(device.bus, device.device, device.function, i);

		printf("%p\n", bars[i]);
	}
	
	uintptr_t ptr = reinterpret_cast<uintptr_t>(bars[5]);
	uintptr_t region = (ptr >> 22) << 22;
	if(!pm.page_table_exists(reinterpret_cast<void*>(ptr))) {
		void * newpagetable = MemoryManager::get().alloc_pages(1);
		pm.new_page_table(newpagetable, reinterpret_cast<void*>(region));
	}
	// Identity map the region (No need for the higher half offset, since this address was already at the higher half)
	// At this point we have mapped 4096 Bytes. HBA AHCI registers (if we have all ports filled) might get up to 4352 bytes.
	pm.map_page(reinterpret_cast<void*>(ptr), reinterpret_cast<void*>(ptr), CACHE_DISABLE | READ_WRITE);
	HBA_MEM * hba = reinterpret_cast<HBA_MEM*>(ptr);
	printf("AHCI enabled: %s\n", hba->ghc & (1 << 31) ? "yes" : "no");
	printf("Capability register: %p\nPorts implemented register: %p\n", hba->cap, hba->pi);
}