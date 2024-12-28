#include <stdio.h>

#include <kernel/memory/page.hpp>
#include <kernel/drivers/tty/ttyman.hpp>
#include <kernel/memory/gdt.hpp>
#include <kernel/drivers/interrupts.hpp>
#include <kernel/assembly/inlineasm.h>
#include <kernel/drivers/serial.hpp>
#include <kernel/drivers/pic.hpp>
#include <kernel/drivers/pit.hpp>
#include <kernel/drivers/ps2.hpp>
#include <kernel/memory/mmanager.hpp>
#include <kernel/drivers/keyboard/keyboard.hpp>
#include <kernel/drivers/mouse.hpp>
#include <kernel/drivers/cursor.hpp>
#include <kernel/cpp/icxxabi.h>
#include <multiboot/multiboot.h>
#include <kernel/drivers/pci/pci.hpp>
#include <kernel/drivers/disk/disk.hpp>

#ifdef DEBUG
#include <kernel/test.hpp>
#endif

void breakpoint() {
	__asm__ volatile("int3");
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

	uint8_t *buffer = new uint8_t[512];
	volatile DiskJob *job = new DiskJob(buffer, 0, 1, 0);
	DiskManager::get().enqueue_job(0, job);
	size_t i = 0;
	while(job->state == DiskJob::WAITING)
		i++;
	printf("Finish: %d\n", i);
	for(size_t i = 0; i < 12; i++) {
		uint8_t p = job->buffer[i];
		printf("%p ", p);
	}
	

	#ifdef DEBUG
	test_paging();
	#endif
	
	printf("Initializing booting sequence\n");
	printf("Finished booting. Giving control to the init process.\n");
	for(;;) {
		__asm__ __volatile__("hlt");
	}
	__cxa_finalize(0);
}
