#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/page.h>
#include <kernel/tty.hpp>
#include <kernel/gdt.h>
#include <kernel/interrupts.hpp>
#include <kernel/inlineasm.h>
#include <kernel/serial.h>
#ifdef DEBUG
#include <kernel/test.hpp>
#endif

void breakpoint() {
	__asm__ volatile("int3");
}

extern "C" void kernel_main(void) {
	init_paging();
	terminal_initialize();
	init_serial();
	
	init_gdt();
	init_idt();
		
	#ifdef DEBUG
	test_paging();
	#endif
	
	
	printf("Initializing booting sequence\n");
	printf("Finished booting. Giving control to the init process.\n");

	for(;;) {
		__asm__ __volatile__("hlt");
	}
}
