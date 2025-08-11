ENTRY (_start)

SECTIONS
{
    . = 0x00100000;
	/* The kernel will live at 3GB + 1MB in the virtual address space, */
	/* which will be mapped to 1MB in the physical address space. */
	/* Note that we page-align the sections. */

	
	.multiboot.data : {
		*(.multiboot.data)
	}

	.multiboot.text : {
		*(.multiboot.text)
	}

	
	. += 0xC0000000;
	_kernel_start = .;
	/* Add a symbol that indicates the start address of the kernel. */
	.text ALIGN (4K) : AT (ADDR (.text) - 0xC0000000)
	{
		*(.text)
	}
	.rodata ALIGN (4K) : AT (ADDR (.rodata) - 0xC0000000)
	{
		*(.rodata)
	}
	.data ALIGN (4K) : AT (ADDR (.data) - 0xC0000000)
	{
		*(.data)
	}
	.bss ALIGN (4K) : AT (ADDR (.bss) - 0xC0000000)
	{
		*(COMMON)
		*(.bss)
		*(.bootstrap_stack)
	}
	.init : AT(ADDR(.init) - 0xC0000000) { *(.init) }
    .fini : AT(ADDR(.fini) - 0xC0000000) { *(.fini) }
    .ctors : AT(ADDR(.ctors) - 0xC0000000) { *(.ctors) }
    .dtors : AT(ADDR(.dtors) - 0xC0000000) { *(.dtors) }

    /* Now mark the real end */
    .end : AT(ADDR(.end) - 0xC0000000) {
        kernel_end = .;
    }
}
