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
#include <multiboot/multiboot.h>

#ifdef DEBUG
#include <kernel/test.hpp>
#endif

void breakpoint() {
	__asm__ volatile("int3");
}

void panic(const char *str) {
	printf("Kernel panic: %s\n", str);
	halt();
}

extern "C" void kernel_main(multiboot_info_t* mbd, unsigned int magic) {
	TTY::get().init();
	if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        panic("invalid magic number!");
    }

	if(!(mbd->flags >> 6 & 0x1)) {
		panic("Invalid memory map");
	}

	for(unsigned i = 0; i < mbd->mmap_length; 
        i += sizeof(multiboot_memory_map_t)) 
    {
        multiboot_memory_map_t* mmmt = 
            (multiboot_memory_map_t*) (mbd->mmap_addr + i);

        printf("Start Addr: %p | Length: %p | Size: %p | Type: %d | ",
            mmmt->addr, mmmt->len, mmmt->size, mmmt->type);

        if(mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) {
			printf("Available\n");
            /* 
             * Do something with this memory block!
             * BE WARNED that some of memory shown as availiable is actually 
             * actively being used by the kernel! You'll need to take that
             * into account before writing to memory!
             */
        } else {
			printf("Not available\n");
		}
    }

	printf("Nothing wrong with memory. Initializing devices... ");
	PIC::get().init();

	Serial::get().init();
	GDT::get().init();
	
	IDT::get().init();
	PIT::get().init(100);
	PagingManager::get().init();
	PS2Controller::get().init();
		
	#ifdef DEBUG
	test_paging();
	#endif
	printf("Done!\n");
	
	printf("Initializing booting sequence\n");
	printf("Finished booting. Giving control to the init process.\n");
	int guard;
	printf("Guard: %p\n", &guard);
	for(;;) {
		__asm__ __volatile__("hlt");
	}
}
