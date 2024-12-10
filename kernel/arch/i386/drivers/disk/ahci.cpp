#include <kernel/drivers/disk/ahci.hpp>
#include <kernel/memory/page.hpp>
#include <stdio.h>
#include <kernel/memory/mmanager.hpp>
#include <string.h>

#include "../../defs/ahci/hba_mem.hpp"
#include "../../defs/ahci/dev_type.hpp"
#include "../../defs/ahci/port_const.hpp"
#include "../../defs/ahci/hba_bits.hpp"
#include "../../defs/ahci/com_header.hpp"

AHCI::AHCI(const PCI::PCIDevice &device) : DiskDriver(device){
	PCI &pci = PCI::get();
	PagingManager& pm = PagingManager::get();
	for(int i = 0; i < 6; i++) {
		bars[i] = pci.getBAR(device.bus, device.device, device.function, i);
	}
	
	uintptr_t ptr = reinterpret_cast<uintptr_t>(bars[5]);
	uintptr_t region = ptr & 0xFFC00000;
	if(!pm.page_table_exists(reinterpret_cast<void*>(ptr))) {
		void * newpagetable = MemoryManager::get().alloc_pages(1);
		pm.new_page_table(newpagetable, reinterpret_cast<void*>(region));
	}
	// Identity map the region (No need for the higher half offset, since this address was already at the higher half)
	pm.map_page(reinterpret_cast<void*>(ptr), reinterpret_cast<void*>(ptr), CACHE_DISABLE | READ_WRITE);
	pm.map_page(reinterpret_cast<void*>(ptr+PAGE_SIZE), reinterpret_cast<void*>(ptr+PAGE_SIZE), CACHE_DISABLE | READ_WRITE);
	HBA_MEM * hba = reinterpret_cast<HBA_MEM*>(ptr);

	size_t AHCI_BASE = reinterpret_cast<size_t>(MemoryManager::get().alloc_pages(76));

	uint32_t pi = hba->pi;
	for(size_t i = 0; i < 32; i++, pi >>= 1) {
		if(pi & 1) {
			AHCI_DEV device = get_device_type(hba->ports + i);
			if(device != NULLDEV)
				rebase_port(&hba->ports[i], i);
			devices.push_back(device);
		}
	}
}

AHCI_DEV AHCI::get_device_type(HBA_PORT* port) {
	uint32_t ssts = port->ssts;

	uint8_t ipm = (ssts >> 8) & 0x0F;
	uint8_t det = ssts & 0x0F;

	if (det != HBA_PORT_DET_PRESENT)	// Check drive status
		return NULLDEV;
	if (ipm != HBA_PORT_IPM_ACTIVE)
		return NULLDEV;

	switch (port->sig)
	{
		case SATA_SIG_ATAPI:
			return SATAPI;
		case SATA_SIG_SEMB:
			return SEMB;
		case SATA_SIG_PM:
			return PM;
		default:
			return SATA;
	}
}

void AHCI::rebase_port(HBA_PORT *port, int portno) {
	
	stop_cmd(port);	// Stop command engine

	// Command list offset: 1K*portno
	// Command list entry size = 32
	// Command list entry maxim count = 32
	// Command list maxim size = 32*32 = 1K per port
	port->clb = AHCI_BASE + (portno<<10);
	port->clbu = 0;
	memset((void*)(port->clb), 0, 1024);
	port->clb -= HIGHER_HALF_OFFSET;

	// FIS offset: 32K+256*portno
	// FIS entry size = 256 bytes per port
	port->fb = AHCI_BASE+ (32<<10) + (portno<<8);
	port->fbu = 0;
	memset((void*)(port->fb), 0, 256);
	port->fb -= HIGHER_HALF_OFFSET;

	// Command table offset: 40K + 8K*portno
	// Command table size = 256*32 = 8K per port
	HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*)(port->clb);
	for (int i=0; i< 32; i++) {
		cmdheader[i].prdtl = 8;	// 8 prdt entries per command table
					// 256 bytes per command table, 64+16+48+16*8
		// Command table offset: 40K + 8K*portno + cmdheader_index*256
		cmdheader[i].ctba = AHCI_BASE + (40<<10) + (portno<<13) + (i<<8);
		cmdheader[i].ctbau = 0;
		memset((void*)cmdheader[i].ctba, 0, 256);
		cmdheader[i].ctba -= HIGHER_HALF_OFFSET;
	}

	start_cmd(port);	// Start command engine
}

void AHCI::start_cmd(HBA_PORT *port) {
	// Wait until CR (bit15) is cleared
	while (port->cmd & HBA_PxCMD_CR);

	// Set FRE (bit4) and ST (bit0)
	port->cmd |= HBA_PxCMD_FRE;
	port->cmd |= HBA_PxCMD_ST;
}

void AHCI::stop_cmd(HBA_PORT *port) {
	// Clear ST (bit0)
	port->cmd &= ~HBA_PxCMD_ST;

	// Clear FRE (bit4)
	port->cmd &= ~HBA_PxCMD_FRE;

	// Wait until FR (bit14), CR (bit15) are cleared
	for(;;) {
		if (port->cmd & HBA_PxCMD_FR)
			continue;
		if (port->cmd & HBA_PxCMD_CR)
			continue;
		break;
	}
}