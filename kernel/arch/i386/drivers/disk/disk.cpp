#include <kernel/drivers/disk/disk.hpp>
#include <kernel/drivers/disk/ahci.hpp>
#include <kernel/drivers/pci/pci.hpp>
#include <stdio.h>
#include <string.h>

#include "../../defs/pci/class_codes.hpp"
#include "../../defs/pci/subclasses/storage.hpp"
#include "../../defs/pci/prog_if/sata_progif.hpp"

diskname::diskname(const char *type, char number) {
	memcpy(this->type, type, sizeof(this->type));
	this->number = number;
}

diskname::operator char *() {
	return reinterpret_cast<char*>(this);
}

DiskManager &DiskManager::get() {
	static DiskManager instance;
	return instance;
}

void DiskManager::init() {
	const size_t ndevices = PCI::get().getDeviceCount();
	PCI::PCIDevice const *devices = PCI::get().getDevices();
	char numberdisk = 'a';
	for(size_t i = 0; i < ndevices; i++) {
		if(devices[i].class_code == STORAGE_CONTROLLER) {
			switch(devices[i].subclass_code) {
				case SATA_CONTROLLER:
					if(devices[i].prog_if == AHCI_PROGIF) {
						diskname dn{"ahci", numberdisk};
						disk_controllers.emplace_back({dn, AHCI(devices[i])});
						char * name = dn;
						printf("New disk: %s\n", name);
						numberdisk++;
					}
					break;
				default:
					printf("Disk type not supported: %d\n", devices[i].subclass_code);
					break;
			}
		}
	}
}