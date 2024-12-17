#pragma once
#include <kernel/drivers/pci/pci.hpp>

class DiskDriver {
	public:
		DiskDriver(const PCI::PCIDevice &device);
		virtual ~DiskDriver() = default;
		virtual bool read(uint64_t lba, uint32_t sector_count, uint16_t *buf) = 0;
		virtual bool write(uint64_t lba, uint32_t sector_count, uint16_t* buf) = 0;

		virtual uint64_t get_disk_size() const = 0;
		virtual uint32_t get_sector_size() const { return 512; };
	protected:
		PCI::PCIDevice device;
		uint32_t bars[6];
};