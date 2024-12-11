#include <kernel/drivers/disk/ahci.hpp>
#include <kernel/memory/page.hpp>
#include <stdio.h>
#include <kernel/memory/mmanager.hpp>
#include <kernel/drivers/pic.hpp>
#include <string.h>

#include "../../defs/ahci/dev_type.hpp"
#include "../../defs/ahci/port_const.hpp"
#include "../../defs/ahci/hba_bits.hpp"
#include "../../defs/ahci/com_header.hpp"

#include "../../defs/pci/command_reg.hpp"
#include "../../defs/pci/offsets/offsets.hpp"
#include "../../defs/pci/offsets/header_0.hpp"


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
	hba = reinterpret_cast<HBA_MEM*>(ptr);

	AHCI_BASE = reinterpret_cast<size_t>(MemoryManager::get().alloc_pages(76));

	uint32_t pi = hba->pi;
	for(size_t i = 0; i < 32; i++, pi >>= 1) {
		if(pi & 1) {
			AHCI_DEV device = get_device_type(hba->ports + i);
			if(device != NULLDEV)
				rebase_port(&hba->ports[i], i);
			devices.push_back(device);
		}
	}

	uint16_t command_register = pci.readConfigWord(device.bus, device.device, device.function, COMMAND);
	command_register |= IO_SPACE;
	command_register |= BUS_MASTER;
	command_register |= MEM_SPACE;
	pci.writeConfigWord(device.bus, device.device, device.function, COMMAND, command_register);

	if((hba->bohc & 0x1)) {
		if(!bios_handoff()) {
			printf("BIOS is a fucker\nReason: BIOS is selfish, doesn't want to hand off device.");
		} else {
			printf("BIOS handoff correct\n");
		}
	} else {
		printf("No need to hand off from BIOS\n");
	}
	if(reset_controller()) {
		printf("AHCI reset successful\n");
	} else {
		printf("AHCI reset went wrong\n");
	}
	setup_interrupts();
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

	// FIS offset: 32K+256*portno
	// FIS entry size = 256 bytes per port
	port->fb = AHCI_BASE+ (32<<10) + (portno<<8);
	port->fbu = 0;
	memset((void*)(port->fb), 0, 256);

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

void AHCI::interrupt_handler(interrupt_frame*) {
	printf("a");
}

bool AHCI::bios_handoff() {
	hba->bohc |= 1;
	uint32_t bohc;
	const int timeout = 1000000; // Timeout in microseconds
    for (int i = 0; i < timeout; i++) {
        bohc = hba->bohc;
        if ((bohc & (1 << 1)) == 0) {
            // BIOS has relinquished control
            return true;
        }
    }
	bohc = hba->bohc;
    if (bohc & (1 << 2)) {
        // Handoff successful
        return true;
    }

    // Handoff failed
    return false;
}

bool AHCI::reset_controller() {
	for (int port = 0; port < devices.size(); ++port) {
		if(devices[port] != NULLDEV) {
			stop_cmd(hba->ports+port);
		}
    }

    // Step 2: Initiate global HBA reset
    uint32_t hctl = hba->ghc;
    hctl |= (1 << 0); // Set HR bit
    hba->ghc = hctl;

    // Step 3: Wait for the reset to complete
    for (int i = 0; i < 1000000; ++i) {
        hctl = hba->ghc;
        if ((hctl & (1 << 0)) == 0) {
            // Reset completed
            return true;
        }
    }

    // Reset failed
    return false;
}

void AHCI::setup_interrupts() {
	uint8_t int_line = PCI::get().readConfigWord(device.bus, device.device, device.function, INTERRUPT_LINE);
	if(int_line == 0xFF) {
		printf("Well we cannot interrupt... fuck\n");
	} else {
		printf("Interrupt: %p\n", int_line);
		PIC::get().IRQ_clear_mask(int_line);
	}
}

void AHCI::enable_ahci_mode() {
	hba->ghc |= 0x80000010;
}