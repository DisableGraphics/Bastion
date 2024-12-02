#include <stdio.h>

#include <kernel/memory/page.hpp>
#include <kernel/tty/ttyman.hpp>
#include <kernel/memory/gdt.hpp>
#include <kernel/drivers/interrupts.hpp>
#include <kernel/assembly/inlineasm.h>
#include <kernel/drivers/serial.hpp>
#include <kernel/drivers/pic.hpp>
#include <kernel/drivers/pit.hpp>
#include <kernel/drivers/ps2.hpp>
#include <kernel/memory/mmanager.hpp>
#include <kernel/keyboard/keyboard.hpp>
#include <kernel/drivers/cursor.hpp>
#include <kernel/cpp/icxxabi.h>
#include <multiboot/multiboot.h>

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
