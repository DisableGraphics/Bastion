#pragma once
#include <kernel/drivers/disk/disk_driver.hpp>

#ifdef __i386__
#include "../arch/i386/defs/ahci/fis_type.hpp"
#include "../arch/i386/defs/ahci/fis_h2d.hpp"
#endif

class AHCI : public DiskDriver {
	public:
		AHCI(const PCI::PCIDevice &device);
	private:
		
};