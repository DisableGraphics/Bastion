#pragma once
#include <kernel/drivers/pci/pci.hpp>

class DiskDriver {
	public:
		DiskDriver(const PCI::PCIDevice &device);
	protected:
		PCI::PCIDevice device;
		uint32_t bars[6];
};