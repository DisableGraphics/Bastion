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

	PIT::get().sleep(1200);

	for(size_t i = 0; i < 512; i += sizeof(int)) {
		printf("%p ", DiskManager::get().buf[i]);
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
