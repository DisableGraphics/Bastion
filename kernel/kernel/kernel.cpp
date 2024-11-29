#include <stdio.h>

#include <kernel/page.hpp>
#include <kernel/tty.hpp>
#include <kernel/gdt.hpp>
#include <kernel/interrupts.hpp>
#include <kernel/inlineasm.h>
#include <kernel/serial.hpp>
#include <kernel/pic.hpp>
#include <kernel/pit.hpp>
#include <kernel/ps2.hpp>
#include <kernel/mmanager.hpp>
#include <kernel/keyboard.hpp>
#include <kernel/cursor.hpp>
#include <multiboot/multiboot.h>

#ifdef DEBUG
#include <kernel/test.hpp>
#endif

void breakpoint() {
	__asm__ volatile("int3");
}

extern "C" void kernel_main(multiboot_info_t* mbd, unsigned int magic) {
	TTY::get().init();
	PagingManager::get().init();
	
	Cursor::get().init();
	Serial::get().init();
	Serial::get().print("ola\n");
	GDT::get().init();
	PIC::get().init();
	IDT::get().init();

	MemoryManager::get().init(mbd, magic);
	
	PIT::get().init(1000);
	PS2Controller::get().init();
	Keyboard::get().init();
	#ifdef DEBUG
	test_paging();
	#endif
	
	printf("Initializing booting sequence\n");
	printf("Finished booting. Giving control to the init process.\n");
	for(;;) {
		__asm__ __volatile__("hlt");
	}
}
