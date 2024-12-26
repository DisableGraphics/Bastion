#include <kernel/drivers/disk/ahci.hpp>
#include <kernel/memory/page.hpp>
#include <stdio.h>
#include <kernel/memory/mmanager.hpp>
#include <kernel/memory/mmio.hpp>
#include <kernel/drivers/pic.hpp>
#include <string.h>

#include "../../defs/ahci/dev_type.hpp"
#include "../../defs/ahci/port_const.hpp"
#include "../../defs/ahci/hba_bits.hpp"
#include "../../defs/ahci/com_header.hpp"
#include "../../defs/ahci/fis_h2d.hpp"
#include "../../defs/ahci/fis_type.hpp"
#include "../../defs/ahci/hba_cmd_tbl.hpp"
#include "../../defs/ahci/hba_prdt_entry.hpp"

#include "../../defs/ata/status_flags.hpp"

#include "../../defs/pci/command_reg.hpp"
#include "../../defs/pci/offsets/offsets.hpp"
#include "../../defs/pci/offsets/header_0.hpp"

#define HBA_PxIS_TFES (1 << 30)

uint8_t int_line;

AHCI::AHCI(const PCI::PCIDevice &device) : DiskDriver(device) {
	PCI &pci = PCI::get();
	PagingManager& pm = PagingManager::get();
	for(int i = 0; i < 6; i++) {
		bars[i] = pci.getBAR(device.bus, device.device, device.function, i);
	}

	uint16_t command_register = pci.readConfigWord(device.bus, device.device, device.function, COMMAND);
	command_register |= IO_SPACE;
	command_register |= BUS_MASTER;
	command_register |= MEM_SPACE;
	pci.writeConfigWord(device.bus, device.device, device.function, COMMAND, command_register);
	
	uintptr_t ptr = reinterpret_cast<uintptr_t>(bars[5]);
	uintptr_t region = ptr & 0xFFC00000;
	if(!pm.page_table_exists(reinterpret_cast<void*>(ptr))) {
		void * newpagetable = MemoryManager::get().alloc_pages(1, CACHE_DISABLE | READ_WRITE);
		pm.new_page_table(newpagetable, reinterpret_cast<void*>(region));
	}
	// Identity map the region (No need for the higher half offset, since this address was already at the higher half)
	pm.map_page(reinterpret_cast<void*>(ptr), reinterpret_cast<void*>(ptr), CACHE_DISABLE | READ_WRITE);
	pm.map_page(reinterpret_cast<void*>(ptr+PAGE_SIZE), reinterpret_cast<void*>(ptr+PAGE_SIZE), CACHE_DISABLE | READ_WRITE);
	hba = reinterpret_cast<volatile HBA_MEM*>(ptr);

	AHCI_BASE = reinterpret_cast<size_t>(MemoryManager::get().alloc_pages(76, CACHE_DISABLE | READ_WRITE));
	enable_ahci_mode();
	uint32_t pi = hba->pi;
	for(size_t i = 0; i < 32; i++, pi >>= 1) {
		if(pi & 1) {
			AHCI_DEV device = get_device_type(hba->ports + i);
			if(device != NULLDEV) {
				rebase_port(&hba->ports[i], i);
				printf("Setting up %d\n", i);
			}
			devices.push_back({i, device});
		}
	}

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
	
	for(size_t i = 0; i < devices.size(); i++) {
		hba->ghc |= 1;
		while(hba->ghc & 1);
		if(devices[i].second != NULLDEV) {
			volatile HBA_PORT * currport = &hba->ports[i];
			reset_port(currport);
			start_command_list_processing(currport);
			port_interrupts(currport);
			printf("Connected: %s\n", is_drive_connected(currport) ? "yes" : "no");
			uint8_t identify_buffer[512];
			DriveInfo dev;
			if (send_identify_ata(currport, &dev, identify_buffer)) {
				printf("Port %d: Drive identified\n", i);
				printf("  Sector size: %d bytes\n", dev.sector_size);
				printf("  Sector count: %d\n", dev.sector_count);
			} else {
				printf("Port %d: IDENTIFY ATA command failed\n", i);
			}
			
		}
	}
}

#define GHC_REG    0x00    // Global Host Control Register offset
#define PORT_CMD   0x18    // Port Command Register offset
#define PORT_SCR   0x28    // Port Status/Control Register offset

// Define the global control register bits
#define GHC_HRST   (1 << 0)   // Host reset bit

// Define the port command register bits
#define PORT_CMD_ST  (1 << 0)  // Start the port
#define PORT_CMD_SPD (1 << 8)  // Port reset (Soft Reset)

#define ATA_CMD_IDENTIFY 0xEC

void AHCI::reset_port(volatile HBA_PORT *port) {
	port->cmd |= PORT_CMD_SPD;
	while(port->cmd & PORT_CMD_SPD);

	port->cmd &= ~PORT_CMD_SPD;  // Clear the reset bit
	port->cmd |= PORT_CMD_ST;   // Set the start bit
}

void AHCI::start_command_list_processing(volatile HBA_PORT* port) {
	port->cmd &= ~PORT_CMD_SPD;
	
	// Set the Start Command bit (ST)
	port->cmd |= PORT_CMD_ST;
}

AHCI_DEV AHCI::get_device_type(volatile HBA_PORT* port) {
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

void AHCI::rebase_port(volatile HBA_PORT *port, int portno) {
	printf("Port %d\n", portno);
	stop_cmd(port);	// Stop command engine

	// Command list offset: 1K*portno
	// Command list entry size = 32
	// Command list entry maxim count = 32
	// Command list maxim size = 32*32 = 1K per port
	port->clb = AHCI_BASE + (portno<<10);
	port->clbu = 0;
	vmemset((void*)(port->clb), 0, 1024);

	// FIS offset: 32K+256*portno
	// FIS entry size = 256 bytes per port
	port->fb = AHCI_BASE+ (32<<10) + (portno<<8);
	port->fbu = 0;
	vmemset((void*)(port->fb), 0, 256);

	// Command table offset: 40K + 8K*portno
	// Command table size = 256*32 = 8K per port
	volatile HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*)(port->clb);
	for (int i=0; i< 32; i++) {
		cmdheader[i].prdtl = 8;	// 8 prdt entries per command table
					// 256 bytes per command table, 64+16+48+16*8cmdslots
		// Command table offset: 40K + 8K*portno + cmdheader_index*256
		cmdheader[i].ctba = AHCI_BASE + (40<<10) + (portno<<13) + (i<<8);
		cmdheader[i].ctbau = 0;
		memset((void*)cmdheader[i].ctba, 0, 256);
		cmdheader[i].ctba -= HIGHER_HALF_OFFSET;
	}
	port->clb -= HIGHER_HALF_OFFSET;
	port->fb -= HIGHER_HALF_OFFSET;
	port->serr = -1;

	start_cmd(port);	// Start command engine
}

void AHCI::start_cmd(volatile HBA_PORT *port) {
	// Wait until CR (bit15) is cleared
	while (port->cmd & HBA_PxCMD_CR);

	// Set FRE (bit4) and ST (bit0)
	port->cmd |= HBA_PxCMD_FRE;
	port->cmd |= HBA_PxCMD_ST;
}

void AHCI::stop_cmd(volatile HBA_PORT *port) {
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
	PIC::get().send_EOI(int_line);
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
		if(devices[port].second != NULLDEV) {
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
	int_line = PCI::get().readConfigWord(device.bus, device.device, device.function, INTERRUPT_LINE);
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

bool AHCI::read(uint64_t lba, uint32_t sector_count, uint16_t *buf) {
	return dma_transfer(false, lba, sector_count, buf);
}

bool AHCI::write(uint64_t lba, uint32_t sector_count, uint16_t* buffer) {
	return dma_transfer(true, lba, sector_count, buffer);
}

uint64_t AHCI::get_disk_size() const {
	return 0;
}

volatile HBA_PORT* AHCI::get_port(uint64_t lba) const {
	if (devices.empty()) return nullptr;
	for(size_t i = 0; i < devices.size(); i++) {
		if(devices[i].second != NULLDEV)
			return &hba->ports[i];
	}
	return nullptr;
}

bool AHCI::dma_transfer(bool is_write, uint64_t lba, uint32_t count, uint16_t *buf) {
	PagingManager &pm = PagingManager::get();
	volatile HBA_PORT* port = get_port(lba);
	if (!port) return false;

	uint32_t startl = lba;
	uint32_t starth = lba >> 32;

	port->is = (uint32_t) -1;		// Clear pending interrupt bits
	int spin = 0; // Spin lock timeout counter
	int slot = find_cmdslot(port);
	if (slot == -1)
		return false;

	volatile HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*)(port->clb+HIGHER_HALF_OFFSET);
	cmdheader += slot;
	cmdheader->cfl = sizeof(FIS_REG_H2D)/sizeof(uint32_t);	// Command FIS size
	cmdheader->w = 0;		// Read from device
	cmdheader->prdtl = (uint16_t)((count-1)>>4) + 1;	// PRDT entries count

	volatile HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL*)(cmdheader->ctba+HIGHER_HALF_OFFSET);
	vmemset(cmdtbl, 0, sizeof(HBA_CMD_TBL) +
 		(cmdheader->prdtl-1)*sizeof(HBA_PRDT_ENTRY));
	int i = 0;
	// 8K bytes (16 sectors) per PRDT
	for (i = 0; i<cmdheader->prdtl-1; i++)
	{
		cmdtbl->prdt_entry[i].dba = (uint32_t) (pm.get_physaddr(buf));
		cmdtbl->prdt_entry[i].dbc = 8*1024-1;	// 8K bytes (this value should always be set to 1 less than the actual value)
		cmdtbl->prdt_entry[i].i = 1;
		buf += 4*1024;	// 4K words
		count -= 16;	// 16 sectors
	}
	// Last entry
	cmdtbl->prdt_entry[i].dba = (uint32_t) (pm.get_physaddr(buf));
	cmdtbl->prdt_entry[i].dbc = (count<<9)-1;	// 512 bytes per sector
	cmdtbl->prdt_entry[i].i = 1;

	// Setup command
	volatile FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);

	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->featurel = 1 | (1 << 2);
	cmdfis->c = 1;	// Command
	cmdfis->command = 0x25;

	cmdfis->lba0 = (uint8_t)startl;
	cmdfis->lba1 = (uint8_t)(startl>>8);
	cmdfis->lba2 = (uint8_t)(startl>>16);
	cmdfis->device = 1<<6;	// LBA mode

	cmdfis->lba3 = (uint8_t)(startl>>24);
	cmdfis->lba4 = (uint8_t)starth;
	cmdfis->lba5 = (uint8_t)(starth>>8);

	cmdfis->countl = count & 0xFF;
	cmdfis->counth = (count >> 8) & 0xFF;

	// The below loop waits until the port is no longer busy before issuing a new command
	while ((port->tfd & (ATA_STATUS_BSY | ATA_STATUS_DRQ)) && spin < 1000000)
	{
		spin++;
	}
	if (spin == 1000000)
	{
		printf("Port is hung\n");
		return false;
	}

	port->ci = 1<<slot;	// Issue command

	// Wait for completion
	while (1)
	{
		// In some longer duration reads, it may be helpful to spin on the DPS bit 
		// in the PxIS port field as well (1 << 5)
		if ((port->ci & (1<<slot)) == 0) 
			break;
		if (port->is & HBA_PxIS_TFES)	// Task file error
		{
			printf("Read disk error\n");
			return false;
		}
	}

	// Check again
	if (port->is & HBA_PxIS_TFES)
	{
		printf("Read disk error\n");
		return false;
	}

	return true;
}

int AHCI::find_cmdslot(volatile HBA_PORT* port)
{
	// If not set in SACT and CI, the slot is free
	uint32_t slots = (port->sact | port->ci);
	for (int i = 0; i < 32; i++, slots >>= 1) {
		if ((slots&1) == 0)
			return i;
	}
	printf("Cannot find free command list entry\n");
	return -1;
}

void AHCI::port_interrupts(volatile HBA_PORT *port) {
	const uint32_t D2H_INTERRUPT_BIT = (1 << 6);
	port->ie |= D2H_INTERRUPT_BIT;
}

bool AHCI::is_drive_connected(volatile HBA_PORT *port) {
	// Read the SATA Status Register (PxSSTS)
	uint32_t ssts = port->ssts;
	uint8_t det = ssts & 0x0F;       // Device Detection bits (0–3)
	uint8_t ipm = (ssts >> 8) & 0x0F; // Interface Power Management bits (8–11)

	// Check if a device is connected and communicating
	if (det == 0x3 && ipm == 0x1) {
		return true; // Drive is connected and active
	}

	return false; // No drive connected or not active
}

bool AHCI::send_identify_ata(volatile HBA_PORT *port, DriveInfo *drive_info, uint8_t *buffer) {
	port->is = (uint32_t)-1; // Clear interrupt status

	// 2. Prepare Command Header
	uint32_t slot = 0; // Assume using slot 0
	volatile HBA_CMD_HEADER *cmd_header = (volatile HBA_CMD_HEADER *)(port->clb+HIGHER_HALF_OFFSET) + slot;
	vmemset(cmd_header, 0, sizeof(HBA_CMD_HEADER));
	cmd_header->cfl = sizeof(FIS_REG_H2D) / 4; // Command FIS length in DWORDs
	cmd_header->w = 0;                         // Write (0 for read)
	cmd_header->prdtl = 1;                     // 1 PRDT entry

	// 3. Prepare Command Table
	volatile HBA_CMD_TBL *cmd_table = (volatile HBA_CMD_TBL *)(cmd_header->ctba+HIGHER_HALF_OFFSET);
	vmemset(cmd_table, 0, sizeof(HBA_CMD_TBL));

	// PRDT entry
	cmd_table->prdt_entry[0].dba = (uint32_t)(uintptr_t)PagingManager::get().get_physaddr(buffer);
	cmd_table->prdt_entry[0].dbc = 511; // 512 bytes - 1
	cmd_table->prdt_entry[0].i = 1;     // Interrupt on completion

	// Command FIS
	volatile FIS_REG_H2D *cmd_fis = (FIS_REG_H2D *)cmd_table->cfis;
	vmemset(cmd_fis, 0, sizeof(FIS_REG_H2D));
	cmd_fis->fis_type = FIS_TYPE_REG_H2D;
	cmd_fis->command = ATA_CMD_IDENTIFY;
	cmd_fis->device = 0; // Master device

	// 4. Issue Command
	port->ci = 1 << slot; // Issue command

	// 5. Wait for completion
	while (port->ci & (1 << slot)) {
		if (port->is & (1 << 30)) { // Check for error
			return false; // Command failed
		}
	}

	// 6. Process Response
	uint16_t *identify_data = (uint16_t *)buffer;
	drive_info->sector_count = ((uint64_t)identify_data[61] << 16) | identify_data[60];
	if (identify_data[106] & (1 << 14)) {
		drive_info->sector_size = 2 * 1024; // 4K sector size
	} else {
		drive_info->sector_size = 512; // Default sector size
	}

	return true;

}