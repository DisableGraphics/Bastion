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
#include <kernel/scheduler/semaphore.hpp>
// C Library headers
#include <stdio.h>
// Tests
#ifdef DEBUG
#include <kernel/test.hpp>
#endif

void breakpoint() {
	__asm__ volatile("int3");
}

int comp = 0;

void test1fn(void*) {
	for(;;) {
		printf("a");
		Scheduler::get().sleep(120);
		Scheduler::get().block(TaskState::WAITING);
	}
}

void test2fn(void* sm) {
	Semaphore* sem = reinterpret_cast<Semaphore*>(sm);
	for(;;) {
		sem->acquire();
		comp--;
		printf("b%d", comp);
		Scheduler::get().sleep(120);
	}
}

void test3fn(void* sm) {
	Semaphore* sem = reinterpret_cast<Semaphore*>(sm);
	for(;;) {
		comp++;
		printf("c%d", comp);
		sem->release();
		Scheduler::get().sleep(120);
	}
}


void test4fn(void* task1) {
	for(;;) {
		printf("d");
		Scheduler::get().unblock(reinterpret_cast<Task*>(task1));
		Scheduler::get().sleep(120);
	}
}

void test5fn(void*) {
	printf("e");
	Scheduler::get().sleep(120);
}

void idle(void*) {
	for(;;) halt();
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
	Semaphore sm1{1, 1};
	Task *idleTask = new Task{idle, nullptr};
	Task *task1 = new Task{test1fn, nullptr}, 
		*task2 = new Task{test2fn, &sm1}, 
		*task3 = new Task{test3fn, &sm1}, 
		*task4 = new Task{test4fn, task1},
		*task5 = new Task{test5fn, nullptr};

	#ifdef DEBUG
	test_paging();
	#endif
	
	printf("Initializing booting sequence\n");
	printf("Finished booting. Giving control to the init process.\n");
	Scheduler::get().append_task(idleTask);
	Scheduler::get().append_task(task1);
	Scheduler::get().append_task(task2);
	Scheduler::get().append_task(task3);
	Scheduler::get().append_task(task4);
	Scheduler::get().append_task(task5);
	Scheduler::get().run();
	for(;;) {
		__asm__ __volatile__("hlt");
	}
	__cxa_finalize(0);
}
