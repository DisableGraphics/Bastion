#include <stdint.h>
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
#include <multiboot/multiboot.h>
#ifdef DEBUG
#include <kernel/test.hpp>
#endif

void breakpoint() {
	__asm__ volatile("int3");
}

extern "C" void kernel_main(multiboot_info_t* mbd, unsigned int magic) {
	TTY::get().init();
	Serial::get().init();
	GDT::get().init();
	PagingManager::get().init();
	MemoryManager::get().init(mbd, magic);
	PIC::get().init();
	IDT::get().init();
	PIT::get().init(100);
	PS2Controller::get().init();
	#ifdef DEBUG
	test_paging();
	#endif
	
	printf("Initializing booting sequence\n");
	printf("Finished booting. Giving control to the init process.\n");
	int guard;
	printf("Guard: %p\n", &guard);
	for(;;) {
		__asm__ __volatile__("hlt");
	}
}
