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

DiskManager::~DiskManager() {
	// Free each disk controller
	for(size_t i = 0; i < disk_controllers.size(); i++) {
		delete disk_controllers[i].second;
	}
}

void DiskManager::init() {
	size_t ndevices = PCI::get().getDeviceCount();
	PCI::PCIDevice const *devices = PCI::get().getDevices();
	char numberdisk = 'a';
	for(size_t i = 0; i < ndevices; i++) {
		if(devices[i].class_code == STORAGE_CONTROLLER) {
			switch(devices[i].subclass_code) {
				case SATA_CONTROLLER:
					if(devices[i].prog_if == AHCI_PROGIF) {
						diskname dn{"ahci", numberdisk};
						char * name = dn;
						AHCI * ahcicont = new AHCI(devices[i]);
						disk_controllers.emplace_back({dn, ahcicont});
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

DiskDriver* DiskManager::get_driver(size_t pos) {
	return disk_controllers[pos].second;
}

Vector<Pair<diskname, DiskDriver*>>& DiskManager::get_disks() {
	return disk_controllers;
}

size_t DiskManager::size() const {
	return disk_controllers.size();
}

bool DiskManager::enqueue_job(size_t diskid, volatile DiskJob* job) {
	if(diskid >= disk_controllers.size()) {
		job->state = DiskJob::ERROR;
		return false;
	}
	
	return disk_controllers[diskid].second->enqueue_job(job);
}

void DiskManager::spin_job(size_t diskid, volatile DiskJob* job) {
	if(diskid >= disk_controllers.size()) {
		job->state = DiskJob::ERROR;
		return;
	}
	
	enqueue_job(diskid, job);
	while(job->state == DiskJob::WAITING);
}