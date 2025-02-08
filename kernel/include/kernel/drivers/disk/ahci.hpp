#pragma once
#include <kernel/hal/drvbase/disk.hpp>
#include <kernel/hal/drvbase/pci.hpp>
#include <kernel/datastr/vector.hpp>
#include <kernel/datastr/pair.hpp>

#ifdef __i386__
#include "../arch/i386/defs/ahci/dev_type.hpp"
#include "../arch/i386/defs/ahci/hba_port.hpp"
#include "../arch/i386/defs/ahci/hba_mem.hpp"
#endif
/**
	\brief Relevant drive info that the ATA_IDENTIFY command returns.
	\details Sector size + sector count. 
 */
typedef struct {
	/// Size of the sector in bytes
	uint16_t sector_size;
	/// Number of sectors in the disk
	uint64_t sector_count;
} DriveInfo;

/**
	\brief AHCI (SATA) disk driver
 */
class AHCI : public virtual hal::Disk, public virtual hal::PCIDriver {
	public:
		AHCI(const hal::PCISubsystemManager::PCIDevice &device);

		// Enqueue job for disk
		bool enqueue_job(volatile hal::DiskJob* job) override;
		// Get how many sectors are in the disk
		uint64_t get_n_sectors() const override;
		// Get sector size of the disk
		uint32_t get_sector_size() const override;
		// Initialise
		void init() override;
		// Enable AHCI DMA interrupts
		void enable_interrupts();
	private:
		// Interrupt handler for DMA
		void handle_interrupt() override;
		// Handoff device from BIOS
		bool bios_handoff();
		// Reset AHCI controller
		bool reset_controller();
		// Setup AHCI DMA interrupts
		void setup_interrupts();
		// Enable AHCI mode
		void enable_ahci_mode();
		// Get device type at port port
		AHCI_DEV get_device_type(volatile HBA_PORT* port);
		// Rebase port port
		void rebase_port(volatile HBA_PORT *port, int portno);
		// Stop command engine of the AHCI disk
		void stop_cmd(volatile HBA_PORT *port);
		// Start command engine of the AHCI disk
		void start_cmd(volatile HBA_PORT *port);
		// Send IDENTIFY_ATA command to the disk so it tells us its info
		bool send_identify_ata(volatile HBA_PORT *port, DriveInfo *drive_info, uint8_t *buffer);
		// Get port for LBA
		volatile HBA_PORT* get_port(uint64_t lba) const;
		// Enable port interrupts for port port
		void port_interrupts(volatile HBA_PORT *port);
		// Check whether there is a drive connected to a port
		bool is_drive_connected(volatile HBA_PORT *port);
		// Resets port
		void reset_port(volatile HBA_PORT *port);
		// Start command list processing
		void start_command_list_processing(volatile HBA_PORT* port);
		// Do a dma transfer to/from the disk for a job
		bool dma_transfer(volatile hal::DiskJob* job);
		// Find free command slot for port
		int find_cmdslot(volatile HBA_PORT* port);

		// AHCI devices
		Vector<Pair<size_t, AHCI_DEV>> devices;
		// Base address of all the pages the driver needs to allocate
		size_t AHCI_BASE;
		// HBA structure of the driver
		volatile HBA_MEM* hba;
		size_t size, sector_size;
		volatile hal::DiskJob* active_jobs[32];		
};