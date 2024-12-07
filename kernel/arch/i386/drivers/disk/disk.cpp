#include <kernel/drivers/disk/disk.hpp>
#include <kernel/drivers/disk/ahci.hpp>
#include <kernel/drivers/pci/pci.hpp>
#include <stdio.h>

#include "../../defs/pci/class_codes.hpp"
#include "../../defs/pci/subclasses/storage.hpp"
#include "../../defs/pci/prog_if/sata_progif.hpp"


DiskManager &DiskManager::get() {
	static DiskManager instance;
	return instance;
}

void DiskManager::init() {
	const size_t ndevices = PCI::get().getDeviceCount();
	PCI::PCIDevice const *devices = PCI::get().getDevices();
	for(size_t i = 0; i < ndevices; i++) {
		if(devices[i].class_code == STORAGE_CONTROLLER) {
			switch(devices[i].subclass_code) {
				case SATA_CONTROLLER:
					if(devices[i].prog_if == AHCI_PROGIF) {
						disk_controllers.push_back(AHCI(devices[i]));
					}
					break;
				default:
					printf("Disk type not supported: %d\n", devices[i].subclass_code);
					break;
			}
		}
	}
}