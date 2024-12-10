#pragma once
#include <kernel/drivers/disk/disk_driver.hpp>
#include <kernel/datastr/vector.hpp>

#ifdef __i386__
#include "../arch/i386/defs/ahci/dev_type.hpp"
#include "../arch/i386/defs/ahci/hba_port.hpp"
#endif

class AHCI : public DiskDriver {
	public:
		AHCI(const PCI::PCIDevice &device);
	private:
		AHCI_DEV get_device_type(HBA_PORT* port);
		void rebase_port(HBA_PORT *port, int portno);
		void stop_cmd(HBA_PORT *port);
		void start_cmd(HBA_PORT *port);
		Vector<AHCI_DEV> devices;
};