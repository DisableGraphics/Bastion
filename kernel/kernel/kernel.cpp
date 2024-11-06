#include <stdio.h>

#include <kernel/page.hpp>
#include <kernel/tty.hpp>
#include <kernel/gdt.hpp>
#include <kernel/interrupts.hpp>
#include <kernel/inlineasm.h>
#include <kernel/serial.hpp>
#include <kernel/pic.hpp>

#ifdef DEBUG
#include <kernel/test.hpp>
#endif

void breakpoint() {
	__asm__ volatile("int3");
}

extern "C" void kernel_main(void) {
	pic.init();
	tty.init();
	
	serial.init();
	gdt.init();
	idt.init();
	page.init();
		
	#ifdef DEBUG
	test_paging();
	#endif
	
	printf("Initializing booting sequence\n");
	printf("Finished booting. Giving control to the init process.\n");
	for(;;) {
		__asm__ __volatile__("hlt");
	}
}
