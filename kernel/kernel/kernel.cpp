// Other required headers
// C++ abi
#include <kernel/cpp/icxxabi.h>
// Multiboot header
#include <multiboot/multiboot.h>

// Memory
#include <kernel/memory/page.hpp>
#include <kernel/memory/gdt.hpp>
#include <kernel/memory/mmanager.hpp>
// Drivers
#include <kernel/drivers/tty/ttyman.hpp>
#include <kernel/drivers/interrupts.hpp>
#include <kernel/drivers/serial.hpp>
#include <kernel/drivers/pic.hpp>
#include <kernel/drivers/pit.hpp>
#include <kernel/drivers/keyboard/keyboard.hpp>
#include <kernel/drivers/mouse.hpp>
#include <kernel/drivers/cursor.hpp>
#include <kernel/drivers/rtc.hpp>
#include <kernel/assembly/inlineasm.h>
#include <kernel/kernel/log.hpp>
// HAL
#include <kernel/hal/managers/ps2manager.hpp>
#include <kernel/hal/managers/diskmanager.hpp>
#include <kernel/hal/managers/irqcmanager.hpp>
#include <kernel/hal/managers/timermanager.hpp>
#include <kernel/hal/managers/clockmanager.hpp>
#include <kernel/hal/managers/pci.hpp>

// Filesystem
#include <kernel/fs/partmanager.hpp>
#include <kernel/fs/fat32.hpp>
// Scheduler
#include <kernel/scheduler/scheduler.hpp>
// Synchronization
#include <kernel/sync/semaphore.hpp>
// C Library headers
#include <stdio.h>
// Tests
#ifdef DEBUG
#include <kernel/test.hpp>
#endif


void idle(void*) {
	for(;;) halt();
}

void gen(void*) {
	auto disks = hal::DiskManager::get().get_disks();
	for(size_t i = 0; i < disks.size(); i++) {
		char * name = disks[i].first;
		uint32_t sector_size = disks[i].second->get_sector_size();
		uint32_t n_sectors = disks[i].second->get_n_sectors();
		uint64_t sizebytes = sector_size * n_sectors;
		uint32_t sizemib = sizebytes / ONE_MEG;
		log(INFO, "New disk: %s. Sector size: %d, Sectors: %d, Size: %dMiB", name, sector_size, n_sectors, sizemib);
	}
	PartitionManager p{0};
	auto parts = p.get_partitions();
	for(size_t i = 0; i < parts.size(); i++) {
		uint64_t sizebytes = parts[i].size * disks[0].second->get_sector_size();
		uint32_t sizemb = sizebytes / ONE_MEG;
		uint32_t type = parts[i].type;
		log(INFO, "Partition: %d %dMiB, Type: %p, start_lba: %d", i, sizemb, type, parts[i].start_lba);
		if(type == 0xc) {
			FAT32 fat{p, i};
		}
	}
}

extern "C" void kernel_main(multiboot_info_t* mbd, unsigned int magic) {
	TTYManager::get().init();
	Cursor::get().init();
	PagingManager::get().init();
	MemoryManager::get().init(mbd, magic);
	Serial::get().init();
	GDT::get().init();
	
	PIC pic;
	IDT::get().init();	
	hal::IRQControllerManager::get().init();
	hal::IRQControllerManager::get().register_controller(&pic);

	PIT pit;
	pit.init();
	hal::TimerManager::get().register_timer(&pit, 1);
	pit.set_is_scheduler_timer(true);

	RTC rtc;
	rtc.init();
	hal::ClockManager::get().set_clock(&rtc);

	Task *idleTask = new Task{idle, nullptr};
	Task *generic = new Task{gen, nullptr};
	Scheduler::get().append_task(idleTask);
	Scheduler::get().append_task(generic);

	hal::PS2SubsystemManager::get().init();

	PS2Keyboard keyb;
	PS2Mouse mouse;

	keyb.init();
	mouse.init();

	hal::PCISubsystemManager::get().init();
	hal::DiskManager::get().init();
	
	Scheduler::get().run();
	for(;;) {
		__asm__ __volatile__("hlt");
	}
	__cxa_finalize(0);
}
