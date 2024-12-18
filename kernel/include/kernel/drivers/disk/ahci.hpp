#pragma once
#include "kernel/drivers/interrupts.hpp"
#include <kernel/drivers/disk/disk_driver.hpp>
#include <kernel/datastr/vector.hpp>
#include <kernel/datastr/pair.hpp>
#include <kernel/sync/semaphore.hpp>

#ifdef __i386__
#include "../arch/i386/defs/ahci/dev_type.hpp"
#include "../arch/i386/defs/ahci/hba_port.hpp"
#include "../arch/i386/defs/ahci/hba_mem.hpp"
#endif

class AHCI : public DiskDriver {
	public:
		AHCI(const PCI::PCIDevice &device);

		bool read(uint64_t lba, uint32_t sector_count, uint16_t *buf) override;
		bool write(uint64_t lba, uint32_t sector_count, uint16_t*) override;
		uint64_t get_disk_size() const override;
	private:
		[[gnu::interrupt]]
		static void interrupt_handler(interrupt_frame*);

		bool bios_handoff();
		bool reset_controller();
		void setup_interrupts();
		void enable_ahci_mode();
		AHCI_DEV get_device_type(HBA_PORT* port);
		void rebase_port(HBA_PORT *port, int portno);
		void stop_cmd(HBA_PORT *port);
		void start_cmd(HBA_PORT *port);
		Vector<Pair<size_t, AHCI_DEV>> devices;
		size_t AHCI_BASE;
		HBA_PORT *get_port(uint64_t lba) const;
		volatile HBA_MEM* hba;
		void reset_port(HBA_PORT *port);
		void start_command_list_processing(HBA_PORT* port);
		bool dma_transfer(bool is_write, uint64_t lba, uint32_t sector_count, uint16_t *buf);
		int find_cmdslot(HBA_PORT *port);
		Semaphore sm{1};
};