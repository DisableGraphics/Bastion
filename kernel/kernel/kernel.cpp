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
#include <kernel/scheduler/user_task.hpp>
#include <kernel/scheduler/kernel_task.hpp>
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
		msg[5] = 0;
		printf("%d: %s (thread #%d)\n", count++, msg, Scheduler::get().get_current_task()->id);
	}
}

void gen(void* arg) {
	log(INFO, "Started gen()");
	hal::VideoDriver* vesa = hal::VideoManager::get().get_driver(0);
	auto disks = hal::DiskManager::get().get_disks();
	printf("a\n");
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
		printf("b\n");
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
			printf("c\n");
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
			printf("d\n");
			log(INFO, "Truncating %s to 64 bytes", filename);
			fat.truncate(filename, 64);
			log(INFO, "Truncating /grub/grubenv to 3 bytes");
			fat.truncate("/grub/grubenv", 3);
			//fat.truncate(filename, 65536);
			log(INFO, "Writing something to %s", filename);
			const char* str = "Never gonna give you up \n\n\n\n\t\thola\naaaaaaasdfdfdf";
			fat.write(filename, 500, strlen(str), str);

			printf("e\n");
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
			printf("f\n");
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

			printf("g\n");

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
			printf("h\n");
			
			DIR dir;
			if(fat.opendir("/", &dir)) {
				printf("This isn't the culprit\n");
				log(INFO, "Opened directory /");
				dirent de;
				while(fat.readdir(&dir, &de)) {
					printf("Directory entry: %s %d %d\n", de.d_name, de.d_type, de.d_ino);
				}

				fat.closedir(&dir);
			}
			
			printf("i\n");
			log(INFO, "Before flush");
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

			// Test directory functionalities
			VFS::get().mount("/", &fat);

			VFS::get().mkdir("/home12");
			VFS::get().touch("/home12/hola1.txt");
			VFS::get().touch("/home12/hola2.txt");
			VFS::get().umount("/");
			VFS::get().mount("/partition1/", &fat);
			printf("/partition1/home12/: \n");
			if(VFS::get().opendir("/partition1/home12", &dir)) {
				struct dirent de;
				while(VFS::get().readdir(&dir, &de)) {
					printf("%s\n", de.d_name);
				}
			}
			VFS::get().closedir(&dir);
			VFS::get().umount("/partition1/");
			VFS::get().mount("/", &fat);

			printf("/: \n");
			if(VFS::get().opendir("/", &dir)) {
				struct dirent de;
				while(VFS::get().readdir(&dir, &de)) {
					printf("%s\n", de.d_name);
				}
			}
			VFS::get().closedir(&dir);
			VFS::get().remove("/home12/hola1.txt");
			VFS::get().remove("/home12/hola2.txt");
			VFS::get().rmdir("/home12");
			fs::BlockCache::get().flush();
			printf("Finished flushing :)\n");
 		}
	}
	
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

int calc(int i);
void fn_user(void*) {
	volatile int i = 0;
	i = calc(i);
	i = calc(i+1);
	i += calc(i+1);
	while(true) i+=1;
}
int calc(int i) {
	int j;
	for(j = 0; j < i; j *= 2);
	return j;
}

void test_malloc() {
	log(ERROR, "----------------------------------------------------------------------");
	log(ERROR, "BEGINNING TEST_MALLOC()");
	for(size_t i = 0; i < 31; i++) {
		uint8_t* buf = (uint8_t*)kmalloc(512);
		printf("%p\n", buf);
		memset(buf, 2, 512);
		kfree((void*)buf);
	}
	log(ERROR, "FINISHED TEST_MALLOC()");
	log(ERROR, "----------------------------------------------------------------------");
}

extern "C" void kernel_main(multiboot_info_t* mbd, unsigned int magic) {
	if(sse2_available()) init_sse2();
	PagingManager::get().init();
	MemoryManager::get().init(mbd, magic);
	Serial::get().init();
	GDT::get().init();
	IDT::get().init();

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
	
	log(INFO, "aVideoDriver vptr = %p", *(size_t*)&vesa);
	multiboot_module_t* mbd_module = reinterpret_cast<multiboot_module_t*>(mbd2->mods_addr + HIGHER_HALF_OFFSET);
	// First module loaded is the ramdisk
	RAMUSTAR ramdisk{reinterpret_cast<uint8_t*>(mbd_module->mod_start+ HIGHER_HALF_OFFSET)};
	
	init_fonts(ramdisk, vesa);
	TTYManager::get().init(&vesa);
	log(INFO, "bVideoDriver vptr = %p", *(size_t*)&vesa);
	
	PIC pic;
	hal::IRQControllerManager::get().init();
	hal::IRQControllerManager::get().register_controller(&pic);
	PIT pit;
	pit.init();
	hal::TimerManager::get().register_timer(&pit, 100*tc::us);
	pit.set_is_scheduler_timer(true);

	log(INFO, "cVideoDriver vptr = %p", *(size_t*)&vesa);

	RTC rtc;
	rtc.init();
	hal::ClockManager::get().set_clock(&rtc);
	log(INFO, "dVideoDriver vptr = %p", *(size_t*)&vesa);

	hal::PS2SubsystemManager::get().init();
	log(INFO, "eVideoDriver vptr = %p", *(size_t*)&vesa);

	PS2Keyboard keyb;
	PS2Mouse mouse;
	keyb.init();
	mouse.init();

	///TODO: Test for userspace, remove later
	size_t useraddr = reinterpret_cast<size_t>(fn_user);
	useraddr = useraddr & 0xFFFFF000;
	//test_malloc();
	printf("fn_ptr: %p, useradd: %p\n", fn_user, useraddr);
	log(INFO, "fVideoDriver vptr = %p", *(size_t*)&vesa);
	
	PagingManager::get().map_page(reinterpret_cast<void*>(useraddr - HIGHER_HALF_OFFSET),
		reinterpret_cast<void*>(useraddr),
		USER | PRESENT);
	PagingManager::get().set_global_options(reinterpret_cast<void*>(useraddr), USER | PRESENT);
	useraddr += PAGE_SIZE;
	PagingManager::get().map_page(reinterpret_cast<void*>(useraddr - HIGHER_HALF_OFFSET),
		reinterpret_cast<void*>(useraddr),
		USER | PRESENT);
	PagingManager::get().set_global_options(reinterpret_cast<void*>(useraddr), USER | PRESENT);

	hal::PCISubsystemManager::get().init();
	hal::DiskManager::get().init();

	log(INFO, "gVideoDriver vptr = %p", *(size_t*)&vesa);
	printf("a\n");

	Task *idleTask = new KernelTask{idle, nullptr};
	Task *generic = new KernelTask{gen, &mouse};
	Pipe p;
	/*Task* testSync1 = new KernelTask{ts1, &p};
	Task* testSync2 = new KernelTask{ts2, &p};
	Task* testSync3 = new KernelTask{ts2, &p};*/
	Task* utask = new UserTask{fn_user, nullptr};
	Scheduler::get().append_task(idleTask);
	Scheduler::get().append_task(generic);
	/*Scheduler::get().append_task(testSync1);
	Scheduler::get().append_task(testSync2);
	Scheduler::get().append_task(testSync3);*/
	Scheduler::get().append_task(utask);
	// Seed RNG
	srand(time(NULL));
	Scheduler::get().run();
	log(INFO, "hVideoDriver vptr = %p", *(size_t*)&vesa);
	printf("text\n");
	for(;;) {
		__asm__ __volatile__("hlt");
	}
	__cxa_finalize(0);
}
