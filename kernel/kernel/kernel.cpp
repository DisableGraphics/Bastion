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
	terminal_initialize();
	init_serial();
	
	init_gdt();
	init_idt();
	init_paging();
		
	#ifdef DEBUG
	test_paging();
	#endif
	
	
	printf("Initializing booting sequence\n");
	
	//printf("sizeof(idt_entry_t): %d\nsizeof(idtr_t) %d\n", sizeof(idt_entry_t), sizeof(idtr_t));
	//printf("GDT Addr: %p\n", gdt);

	/*for(int i = 0; i < 40; i+=8) {
		printf("%p %p %p %p %p %p %p %p\n", idt[i], idt[i+1], idt[i+2], idt[i+3], idt[i+4], idt[i+5], idt[i+6], idt[i+7]);
	}*/

	//printf("Checking this thing: %d", 0/0);
	//map_page((int *)0x3000, (int *)0x4000, 0x3);
	//printf("Finished booting. Giving control to the init process.");

	for(;;) {
		asm("hlt");
	}
}
