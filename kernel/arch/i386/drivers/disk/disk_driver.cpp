#include <kernel/drivers/disk/disk_driver.hpp>

DiskDriver::DiskDriver(const PCI::PCIDevice &device) {
	this->device = device;
}