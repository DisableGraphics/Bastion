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
#include <kernel/drivers/rtc.hpp>
#include <kernel/drivers/vesa/vesa.hpp>
// HAL
#include <kernel/hal/managers/ps2manager.hpp>
#include <kernel/hal/managers/diskmanager.hpp>
#include <kernel/hal/managers/irqcmanager.hpp>
#include <kernel/hal/managers/timermanager.hpp>
#include <kernel/hal/managers/clockmanager.hpp>
#include <kernel/hal/managers/pci.hpp>
#include <kernel/hal/managers/videomanager.hpp>
// Math
#include <kernel/cpp/max.hpp>
#include <kernel/cpp/min.hpp>
// Filesystem
#include <kernel/fs/partmanager.hpp>
#include <kernel/fs/fat32.hpp>
#include <kernel/fs/cache.hpp>
#include <kernel/fs/ramustar.hpp>
// Scheduler
#include <kernel/scheduler/scheduler.hpp>
// Synchronization
#include <kernel/sync/semaphore.hpp>
#include <kernel/sync/pipe.hpp>
// C Library headers
#include <stdio.h>
// Other
#include <kernel/assembly/inlineasm.h>
#include <kernel/kernel/log.hpp>
// Vector instructions
#include <kernel/vector/sse2.h>
// SSFN
#include <ssfn/ssfn.h>
// VFS
#include <kernel/vfs/vfs.hpp>
// Tests
#ifdef DEBUG
#include <kernel/test.hpp>
#endif


void idle(void*) {
	for(;;) halt();
}

void generate_pointer(uint32_t* pixels) {
	for(size_t y = 0; y < 16; y++) {
		const size_t offset = y * 16;
		for(size_t x = 0; x < 16; x++) {
			if(((x == 15 || x == 0 || y == 0 || y == 15) && x <= y) || x == y) {
				pixels[offset + x] = 0xFFFFFFFF;
			} else if(x < y) {
				pixels[offset + x] = 0x000000FF;
			} else {
				pixels[offset + x] = 0;
			}
		}
	}
}

void ts1(void* pipe) {
	Pipe* p = reinterpret_cast<Pipe*>(pipe);
	char msg[] = "hola0";
	const size_t msglen = strlen(msg);
	for(int i = 0; i < 20; i++) {
		msg[4] = (i % 10) + '0';
		log(INFO, "Writing iteration: %d", i);
		p->write(msg, msglen);
	}
}

void ts2(void* pipe) {
	static int count = 0;
	Pipe* p = reinterpret_cast<Pipe*>(pipe);
	char msg[6];
	const size_t msglen = 5;
	for(int i = 0; i < 10; i++) {
		log(INFO, "Reading iteration: %d", i);
		p->read(msg, msglen);
		printf("%d: %s (thread #%d)\n", count++, msg, Scheduler::get().get_current_task()->id);
	}
}

void gen(void* arg) {
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
			VFS::get().mount("/partition/", &fat);
			struct stat st;
			char buffer[16];
			const char* filename = "/data/test.txt";
			log(INFO, "Reading %s", filename);
			tc::timertime time = hal::TimerManager::get().get_timer(0)->ellapsed();
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
			tc::timertime time2 = hal::TimerManager::get().get_timer(0)->ellapsed();
			printf("Took %dms\n", (time2 - time) / (tc::ms));
			// Test VFS
			int inode = VFS::get().open("/partition/data/test.txt", 0);
			
			char c = 0;
			int nread = 0;
			VFS::get().truncate(inode, 256);
			VFS::get().stat(inode, &st);

			VFS::get().close(inode);
			VFS::get().umount("/partition/");
			inode = VFS::get().open("/partition/data/test.txt", 0);
			if(inode == -1)
				printf("Umount works properly: %p\n", (void*)inode);
			else
			 	printf("Umount doesn't work: %p\n", (void*)inode);
 		}
	}
	hal::VideoDriver* vesa = hal::VideoManager::get().get_driver(0);
	vesa->clear({0,0,0,0});
	PS2Mouse* mouse = reinterpret_cast<PS2Mouse*>(arg);
	int mx = 0, my = 0;
	const int width = vesa->get_width();
	const int height = vesa->get_height();
	constexpr int POINTER_SIZE = 16;
	uint32_t pointer[POINTER_SIZE*POINTER_SIZE];
	generate_pointer(pointer);
	while(true) {
		while(mouse->events_queue.size() > 0) {
			auto pop = mouse->events_queue.pop();
			mx = max(0, min(mx + pop.xdisp, width - 1));
			my = max(0, min(my - pop.ydisp, height - 1));
			if(mouse->events_queue.size() == 0) {
				vesa->clear({0,0,0,0});
				log(INFO, "Drawing at %d %d", mx, my);
				vesa->draw_pixels(mx, my, POINTER_SIZE,POINTER_SIZE, (uint8_t*)pointer);
				vesa->flush();
			}
		}
		Scheduler::get().sleep(16*tc::ms);
	}
}

void init_fonts(RAMUSTAR& ramdisk, VESADriver& vesa) {
	// Load font data from the ramdisk
	struct stat fontstat;
	ramdisk.stat("ramdisk/console.sfn", &fontstat);
	uint8_t* fonts = new uint8_t[fontstat.st_size];
	ramdisk.read("ramdisk/console.sfn", 0, fontstat.st_size, reinterpret_cast<char*>(fonts));
	vesa.set_fonts(fonts);
}

extern "C" void kernel_main(multiboot_info_t* mbd, unsigned int magic) {
	if(sse2_available()) init_sse2();
	PagingManager::get().init();
	MemoryManager::get().init(mbd, magic);
	Serial::get().init();
	GDT::get().init();
	

	multiboot_info_t* mbd2 = reinterpret_cast<multiboot_info_t*>(reinterpret_cast<uintptr_t>(mbd) + HIGHER_HALF_OFFSET);

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
	hal::VideoManager::get().register_driver(&vesa);

	multiboot_module_t* mbd_module = reinterpret_cast<multiboot_module_t*>(mbd2->mods_addr + HIGHER_HALF_OFFSET);
	// First module loaded is the ramdisk
	RAMUSTAR ramdisk{reinterpret_cast<uint8_t*>(mbd_module->mod_start+ HIGHER_HALF_OFFSET)};
	
	init_fonts(ramdisk, vesa);
	TTYManager::get().init(&vesa);
	
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

	hal::PS2SubsystemManager::get().init();

	PS2Keyboard keyb;
	PS2Mouse mouse;
	keyb.init();
	mouse.init();

	Task *idleTask = new Task{idle, nullptr};
	Task *generic = new Task{gen, &mouse};
	Pipe p;
	Task* testSync1 = new Task{ts1, &p};
	Task* testSync2 = new Task{ts2, &p};
	Task* testSync3 = new Task{ts2, &p};
	Scheduler::get().append_task(idleTask);
	Scheduler::get().append_task(generic);
	Scheduler::get().append_task(testSync1);
	Scheduler::get().append_task(testSync2);
	Scheduler::get().append_task(testSync3);
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
