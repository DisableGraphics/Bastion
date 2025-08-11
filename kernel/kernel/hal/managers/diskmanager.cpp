#include <kernel/hal/managers/diskmanager.hpp>
#include <kernel/drivers/disk/ahci.hpp>
#include <kernel/hal/managers/pci.hpp>
#include <kernel/kernel/log.hpp>
#include <stdio.h>
#include <string.h>

#include "../arch/i386/defs/pci/class_codes.hpp"
#include "../arch/i386/defs/pci/subclasses/storage.hpp"
#include "../arch/i386/defs/pci/prog_if/sata_progif.hpp"
#include <kernel/sync/semaphore.hpp>

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
	init_pci_disks();
}

void hal::DiskManager::init_pci_disks() {
	size_t ndevices = hal::PCISubsystemManager::get().getDeviceCount();
	hal::PCISubsystemManager::PCIDevice const *devices = hal::PCISubsystemManager::get().getDevices();
	char numberdisk = 'a';
	for(size_t i = 0; i < ndevices; i++) {
		if(devices[i].class_code == STORAGE_CONTROLLER) {
			switch(devices[i].subclass_code) {
				case SATA_CONTROLLER:
					if(devices[i].prog_if == AHCI_PROGIF) {
						diskname dn{"ahci", numberdisk};
						char * name = dn;
						AHCI * ahcicont = new AHCI(devices[i]);
						disk_controllers.emplace_back(dn, ahcicont);
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
	log(INFO, "Enqueued job into disk %d", diskid);
	return disk_controllers[diskid].second->enqueue_job(job);
}

void on_finish_sleep_job(volatile void* args) {
	log(INFO, "sem->release() called");
	Semaphore* sem = const_cast<Semaphore*>(reinterpret_cast<volatile Semaphore*>(args));
	sem->release();
}

void hal::DiskManager::sleep_job(size_t diskid, volatile DiskJob* job) {
	if(diskid >= disk_controllers.size()) {
		job->state = DiskJob::ERROR;
		return;
	}
	Semaphore *sem = new Semaphore{1, 1};
	job->on_finish = on_finish_sleep_job;
	job->on_finish_args = sem;
	// Do not block on error
	job->on_error = job->on_finish;
	job->on_error_args = job->on_finish_args;
	enqueue_job(diskid, job);
	log(INFO, "sem.acquire()");
	sem->acquire();
	delete sem;
}