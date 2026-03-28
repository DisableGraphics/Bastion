OUTPUT_FORMAT(elf64-x86-64)

ENTRY(_start)

SECTIONS
{
	. = 0x800000;
	.text : { *(.text*) }
	. = ALIGN(CONSTANT(MAXPAGESIZE));
	.rodata : { *(.rodata*) }
	. = ALIGN(CONSTANT(MAXPAGESIZE));
	.data : { *(.data*) }
	. = ALIGN(CONSTANT(MAXPAGESIZE));
	.bss : { *(.bss*) }
}