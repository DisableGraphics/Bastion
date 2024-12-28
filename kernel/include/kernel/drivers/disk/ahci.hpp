#pragma once
#include "kernel/drivers/interrupts.hpp"
#include <kernel/drivers/disk/disk_driver.hpp>
#include <kernel/datastr/vector.hpp>
#include <kernel/datastr/pair.hpp>
#include <kernel/sync/semaphore.hpp>
#include <kernel/memory/mmio.hpp>

#ifdef __i386__
#include "../arch/i386/defs/ahci/dev_type.hpp"
#include "../arch/i386/defs/ahci/hba_port.hpp"
#include "../arch/i386/defs/ahci/hba_mem.hpp"
#endif

typedef struct {
	uint16_t sector_size;
	uint64_t sector_count;
} DriveInfo;

/**
	\brief AHCI (SATA) disk driver
 */
class AHCI : public DiskDriver {
	public:
		AHCI(const PCI::PCIDevice &device);

		bool enqueue_job(volatile DiskJob* job) override;
		uint64_t get_n_sectors() const override;
		uint32_t get_sector_size() const override;
		void init();

		void enable_interrupts();
	private:
		[[gnu::interrupt]]
		static void interrupt_handler(interrupt_frame*);

		bool bios_handoff();
		bool reset_controller();
		void setup_interrupts();
		void enable_ahci_mode();
		AHCI_DEV get_device_type(volatile HBA_PORT* port);
		void rebase_port(volatile HBA_PORT *port, int portno);
		void stop_cmd(volatile HBA_PORT *port);
		void start_cmd(volatile HBA_PORT *port);
		bool send_identify_ata(volatile HBA_PORT *port, DriveInfo *drive_info, uint8_t *buffer);
		Vector<Pair<size_t, AHCI_DEV>> devices;
		size_t AHCI_BASE;
		volatile HBA_PORT* get_port(uint64_t lba) const;
		volatile HBA_MEM* hba;
		void port_interrupts(volatile HBA_PORT *port);
		bool is_drive_connected(volatile HBA_PORT *port);
		void reset_port(volatile HBA_PORT *port);
		void start_command_list_processing(volatile HBA_PORT* port);
		bool dma_transfer(volatile DiskJob* job);
		int find_cmdslot(volatile HBA_PORT* port);

		size_t size, sector_size;
		volatile DiskJob* active_jobs[32];
};