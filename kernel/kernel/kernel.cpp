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
#include <kernel/drivers/vesa.hpp>
// HAL
#include <kernel/hal/managers/ps2manager.hpp>
#include <kernel/hal/managers/diskmanager.hpp>
#include <kernel/hal/managers/irqcmanager.hpp>
#include <kernel/hal/managers/timermanager.hpp>
#include <kernel/hal/managers/clockmanager.hpp>
#include <kernel/hal/managers/pci.hpp>
#include <kernel/hal/managers/videomanager.hpp>

// Filesystem
#include <kernel/fs/partmanager.hpp>
#include <kernel/fs/fat32.hpp>
#include <kernel/fs/cache.hpp>
// Scheduler
#include <kernel/scheduler/scheduler.hpp>
// Synchronization
#include <kernel/sync/semaphore.hpp>
// C Library headers
#include <stdio.h>
// Other
#include <kernel/assembly/inlineasm.h>
#include <kernel/kernel/log.hpp>
// Vector instructions
#include <kernel/vector/sse2.h>
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
			char buffer[16];
			const char* filename = "/data/test.txt";
			log(INFO, "Reading %s", filename);
			if(fat.read(filename, 0, 15, buffer) != -1) {
				buffer[15] = 0;
				log(INFO, "Got this: %s", buffer);
			} else {
				log(INFO, "Could not read from /data/test.txt");
			}
			char buffer2[2048];
			int read;
			log(INFO, "Reading 2 %s", filename);
			if((read = fat.read(filename, 0, 2048, buffer2)) != -1) {
				buffer2[2047] = 0;
				log(INFO, "Read: %d bytes", read);
				log(INFO, "Length: %d", strlen(buffer2));
				log(INFO, "Got this:\n%s", buffer2);
			} else {
				log(INFO, "Could not read from /grub/grubenv");
			}
			log(INFO, "stat()ing %s", filename);
			stat st;
			if(fat.stat(filename, &st) != -1) {
				log(INFO, "Size of %s: %d bytes", filename, st.st_size);
				log(INFO, "Creation date: %d", st.st_ctime);
				log(INFO, "Accessed date: %d", st.st_atime);
			}
			log(INFO, "Truncating %s to 64 bytes", filename);
			fat.truncate(filename, 64);
			log(INFO, "Truncating /grub/grubenv to 3 bytes");
			fat.truncate("/grub/grubenv", 3);
			//fat.truncate(filename, 65536);
			log(INFO, "Writing something to %s", filename);
			const char* str = "Never gonna give you up \n\n\n\n\t\thola\naaaaaaasdfdfdf";
			fat.write(filename, 500, strlen(str), str);

			log(INFO, "Truncating %p to 0 bytes");
			fat.truncate(filename, 0);
			log(INFO, "Truncating %p to 512 bytes");
			fat.truncate(filename, 512);
			log(INFO, "Truncating %p to 0 bytes");
			fat.truncate(filename, 0);
			log(INFO, "Truncating %p to 1026 bytes");
			fat.truncate(filename, 1026);
			log(INFO, "Writing something to %s", filename);
			const char* str2 = "Hey hey hey hey hel\n\nhola";
			fat.write(filename, 0, strlen(str2), str2);
			log(INFO, "Creating /data/hll.txt");
			fat.touch("/data/hll.txt");
			log(INFO, "Writing to /data/hll.txt");
			fat.write("/data/hll.txt", 0, strlen(str2), str2);
			log(INFO, "Creating /data/hll.txt again");
			fat.touch("/data/hll.txt");
			char testbuf[strlen(str2)];
			if(fat.read("/data/hll.txt", 0, strlen(str2), testbuf) != -1) {
				if(!memcmp(testbuf, str2, strlen(str2))) {
					log(INFO, "No modifications. Everything went well :)");
				} else {
					log(ERROR, "There have been modifications in /data/hll.txt. \"%s\" vs \"%s\"", testbuf, str2);
				}
			} else {
				log(INFO, "Could not read from /data/hll.txt");
			}
			log(INFO, "Trying to create a file with a huge name");
			if(fat.touch("/data/236147861327463247812367846123746871236478162374612837467812364781627834618.txt")) {
				log(INFO, "Could create the file with a long name");
			}
			log(INFO, "Try to create a directory");
			if(fat.mkdir("/data/carpetahola")) {
				log(INFO, "Created /data/carpetahola");
			}
			if(fat.touch("/data/carpetahola/hola.txt")) {
				log(INFO, "Created /data/carpetahola/hola.txt");
			}
			fat.write("/data/carpetahola/hola.txt", 0, 12, "Hello worllllldddd");
			if(fat.remove("/data/carpetahola/hola.txt")) {
				log(INFO, "Deleted /data/carpetahola/hola.txt sucessfully");
			}

			if(fat.mkdir("/data/carpetaadios")) {
				log(INFO, "Created directory /data/carpetaadios");
			}
			if(fat.rmdir("/data/carpetaadios")) {
				log(INFO, "Deleted directory /data/carpetaadios");
			}
			if(fat.mkdir("/home")) {
				log(INFO, "Created /home");
			}
			if(fat.rename("/data/carpetahola", "/data/carpetahola2")) 
				log(INFO, "Renamed directory /data/carpetahola /data/carpetahola2");
			if(fat.rename("/data/carpetahola2", "/hello")) 
				log(INFO, "Renamed directory /data/carpetahola2 /hello");
			DIR dir;
			if(fat.opendir("/", &dir)) {
				log(INFO, "Opened directory /");
				dirent de;
				while(fat.readdir(&dir, &de)) {
					log(INFO, "Directory entry: %s %d %d", de.d_name, de.d_type, de.d_ino);
				}

				fat.closedir(&dir);
			}
			fs::BlockCache::get().flush();
 		}
	}
}

extern "C" void kernel_main(multiboot_info_t* mbd, unsigned int magic) {
	if(sse2_available()) init_sse2();
	TTYManager::get().init();
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
	hal::TimerManager::get().register_timer(&pit, 100*tc::us);
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

	multiboot_info_t* mbd2 = reinterpret_cast<multiboot_info_t*>(reinterpret_cast<uintptr_t>(mbd) + HIGHER_HALF_OFFSET);

	log(INFO, "MBD address: %p", mbd2);
	auto fbsize = (mbd2->framebuffer_bpp/8) * mbd2->framebuffer_height * mbd2->framebuffer_pitch;
	log(INFO, "Framebuffer size: %p", fbsize);
	auto fbsize_pages = (fbsize + PAGE_SIZE - 1)/PAGE_SIZE;
	auto fbsize_regions = (fbsize + REGION_SIZE - 1)/REGION_SIZE;
	if(!PagingManager::get().page_table_exists(reinterpret_cast<void*>(mbd2->framebuffer_addr))) {
		for(size_t region = 0; region < fbsize_regions; region++) {
			void* newpagetable = MemoryManager::get().alloc_pages(1, CACHE_DISABLE | READ_WRITE);
			void* fbaddroff = reinterpret_cast<void*>(mbd2->framebuffer_addr + region*REGION_SIZE);
			PagingManager::get().new_page_table(newpagetable, 
				fbaddroff);
			for(auto pages = 0; pages < PAGE_SIZE; pages++) {
				void* fbaddroff_p = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(fbaddroff) + pages*PAGE_SIZE);
				PagingManager::get().map_page(fbaddroff_p, 
					fbaddroff_p, 
					CACHE_DISABLE | READ_WRITE);
			}
		}
	}
	
	VESADriver vesa{
		reinterpret_cast<uint8_t*>(mbd2->framebuffer_addr),
		mbd2->framebuffer_width,
		mbd2->framebuffer_height,
		mbd2->framebuffer_pitch,
		mbd2->framebuffer_bpp,
		mbd2->framebuffer_red_field_position,
		mbd2->framebuffer_green_field_position,
		mbd2->framebuffer_blue_field_position,
		mbd2->framebuffer_red_mask_size,
		mbd2->framebuffer_green_mask_size,
		mbd2->framebuffer_blue_mask_size
	};
	vesa.init();
	size_t vesaid = hal::VideoManager::get().register_driver(&vesa);
	size_t ellapsed = hal::TimerManager::get().get_timer(0)->ellapsed();
	for(size_t j = 0; j < 60; j++) {
		size_t frame_time = hal::TimerManager::get().get_timer(0)->ellapsed();
		for(size_t y = 0; y < mbd2->framebuffer_height; y++) {
			for(size_t x = 0; x < mbd2->framebuffer_width; x++) {
				hal::VideoManager::get().draw_pixel(vesaid, x, y, {(int)x, (int)y, (int)(x+y+(1<<(j & 31)))});
			}
		}
		hal::VideoManager::get().flush(vesaid);
		hal::VideoManager::get().clear(vesaid);
	}
	size_t ellapsed2 = hal::TimerManager::get().get_timer(0)->ellapsed();
	log(INFO, "Before: %d vs After: %d", ellapsed, ellapsed2);

	hal::PCISubsystemManager::get().init();
	hal::DiskManager::get().init();
	// Seed RNG
	srand(time(NULL));
	Scheduler::get().run();
	for(;;) {
		__asm__ __volatile__("hlt");
	}
	__cxa_finalize(0);
}
