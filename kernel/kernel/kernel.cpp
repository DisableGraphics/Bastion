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
#include <kernel/drivers/ps2.hpp>
#include <kernel/drivers/keyboard/keyboard.hpp>
#include <kernel/drivers/mouse.hpp>
#include <kernel/drivers/cursor.hpp>
#include <kernel/drivers/pci/pci.hpp>
#include <kernel/drivers/disk/disk.hpp>
#include <kernel/drivers/rtc.hpp>
#include <kernel/assembly/inlineasm.h>
// Filesystem
#include <kernel/fs/partmanager.hpp>
#include <kernel/fs/fat32.hpp>
// Scheduler
#include <../arch/i386/scheduler/interface.hpp>
#include <kernel/scheduler/scheduler.hpp>
// C Library headers
#include <stdio.h>
// Tests
#ifdef DEBUG
#include <kernel/test.hpp>
#endif

void breakpoint() {
	__asm__ volatile("int3");
}

void test_kernel_task() {
	for(;;) {
		Scheduler::get().unlock();
		printf("a");
        Scheduler::get().lock();
        Scheduler::get().schedule();
    }

}

void task_2_fn() {
	while(true) {
		Scheduler::get().unlock();
		printf("b");
		Scheduler::get().block_task(TaskState::PAUSED);
		Scheduler::get().lock();
        Scheduler::get().schedule();
	}
}

void task_3_fn() {
	while(true) {
		Scheduler::get().unlock();
		printf("c");
		Scheduler::get().sleep(120);
		
		Scheduler::get().lock();
        Scheduler::get().schedule();
	}
}

void idle_task() {
	while(true) {
		halt();
	}
}

extern "C" void kernel_main(multiboot_info_t* mbd, unsigned int magic) {
	TTYManager::get().init();
	Cursor::get().init();
	PagingManager::get().init();
	MemoryManager::get().init(mbd, magic);
	Serial::get().init();
	GDT::get().init();
	
	PIC::get().init();
	IDT::get().init();
	
	PIT::get().init(1000);
	RTC::get().init();
	PS2Controller::get().init();
	Keyboard::get().init();
	Mouse::get().init();
	PCI::get().init();
	DiskManager::get().init();

	auto disks = DiskManager::get().get_disks();
	for(size_t i = 0; i < disks.size(); i++) {
		char * name = disks[i].first;
		uint32_t sector_size = disks[i].second->get_sector_size();
		uint32_t n_sectors = disks[i].second->get_n_sectors();
		uint64_t sizebytes = sector_size * n_sectors;
		uint32_t sizemib = sizebytes / ONE_MEG;
		printf("New disk: %s. Sector size: %d, Sectors: %d, Size: %dMiB\n", name, sector_size, n_sectors, sizemib);
	}
	PartitionManager p{0};
	auto parts = p.get_partitions();
	for(size_t i = 0; i < parts.size(); i++) {
		uint64_t sizebytes = parts[i].size * disks[0].second->get_sector_size();
		uint32_t sizemb = sizebytes / ONE_MEG;
		uint32_t type = parts[i].type;
		printf("Partition: %d %dMiB, Type: %p, start_lba: %d\n", i, sizemb, type, parts[i].start_lba);
		if(type == 0xc) {
			FAT32 fat{p, i};
		}
	}
	Task maintask, task2, task3;
	maintask.esp = get_esp();
	maintask.cr3 = read_cr3();

	void * teststack = kcalloc(16384, 1);
	task2.esp = reinterpret_cast<uint32_t>(teststack);
	task2.cr3 = read_cr3();
	Scheduler::get().prepare_task(task2, reinterpret_cast<void*>(task_2_fn));

	void * teststack2 = kcalloc(16384, 1);
	task3.esp = reinterpret_cast<uint32_t>(teststack2);
	task3.cr3 = read_cr3();
	Scheduler::get().prepare_task(task3, reinterpret_cast<void*>(task_3_fn));

	Scheduler::get().set_first_task(&maintask);
	Scheduler::get().append_task(&task2);
	Scheduler::get().append_task(&task3);

	#ifdef DEBUG
	test_paging();
	#endif
	
	printf("Initializing booting sequence\n");
	printf("Finished booting. Giving control to the init process.\n");
	Scheduler::get().schedule();
	for(;;) {
		__asm__ __volatile__("hlt");
	}
	__cxa_finalize(0);
}
