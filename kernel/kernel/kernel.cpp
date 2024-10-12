#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/page.h>
#include <kernel/tty.hpp>
#include <kernel/gdt.h>
#include <kernel/interrupts.hpp>
#include <kernel/inlineasm.h>
#include <kernel/serial.hpp>
#ifdef DEBUG
#include <kernel/test.hpp>
#endif

void breakpoint() {
	__asm__("int3");
}

extern "C" void kernel_main(void) {
	
	init_gdt();
	init_idt();
	init_serial();
	init_paging();

	#ifdef DEBUG
	test_paging();
	#endif
	
	terminal_initialize();
	printf("Initializing booting sequence\n");
	printf("Checking: %d %d\n", sizeof(idt_entry_t), sizeof(idtr_t));
	
	//breakpoint();
	//map_page((int *)0x3000, (int *)0x4000, 0x3);
	printf("Finished booting. Giving control to the init process.");
	for(;;) {
		asm("hlt");
	}
}
