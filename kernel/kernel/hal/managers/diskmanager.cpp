#include <kernel/hal/managers/diskmanager.hpp>
#include <kernel/drivers/disk/ahci.hpp>
#include <kernel/drivers/pci/pci.hpp>
#include <kernel/kernel/log.hpp>
#include <stdio.h>
#include <string.h>

#include "../arch/i386/defs/pci/class_codes.hpp"
#include "../arch/i386/defs/pci/subclasses/storage.hpp"
#include "../arch/i386/defs/pci/prog_if/sata_progif.hpp"

hal::DiskManager &hal::DiskManager::get() {
	static DiskManager instance;
	return instance;
}

hal::DiskManager::~DiskManager() {
	// Free each disk controller
	for(size_t i = 0; i < disk_controllers.size(); i++) {
		delete disk_controllers[i].second;
	}
}

void hal::DiskManager::init() {
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
					log(INFO, "Disk type not supported: %d\n", devices[i].subclass_code);
					break;
			}
		}
	}
}

hal::Disk* hal::DiskManager::get_driver(size_t pos) {
	return disk_controllers[pos].second;
}

Vector<Pair<diskname, hal::Disk*>>& hal::DiskManager::get_disks() {
	return disk_controllers;
}

size_t hal::DiskManager::size() const {
	return disk_controllers.size();
}

bool hal::DiskManager::enqueue_job(size_t diskid, volatile DiskJob* job) {
	if(diskid >= disk_controllers.size()) {
		job->state = DiskJob::ERROR;
		return false;
	}
	
	return disk_controllers[diskid].second->enqueue_job(job);
}

void hal::DiskManager::spin_job(size_t diskid, volatile DiskJob* job) {
	if(diskid >= disk_controllers.size()) {
		job->state = DiskJob::ERROR;
		return;
	}
	
	enqueue_job(diskid, job);
	while(job->state == DiskJob::WAITING);
}