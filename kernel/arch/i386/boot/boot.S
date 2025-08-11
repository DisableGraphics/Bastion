.set MULTIBOOT2_MAGIC, 0xe85250d6
.set ARCHITECTURE, 0
.set HEADER_ALIGN, 8

.section .multiboot.data, "a", @progbits
.align 8

multiboot_header:
	.long MULTIBOOT2_MAGIC                         # magic
	.long ARCHITECTURE                             # architecture
	.long multiboot_header_end - multiboot_header  # total header length
	.long -(MULTIBOOT2_MAGIC + ARCHITECTURE + (multiboot_header_end - multiboot_header))  # checksum

	# Information Request Tag (type 1)
	.short 1                                       # tag type: INFORMATION_REQUEST
	.short 0                                       # flags
	.long 16                                       # size of this tag (header + 2 requests + padding)

	.long 4                                        # request meminfo (tag type 4)
	.long 8											# request framebuffer

	# Request Framebuffer
	.short 5 # tag type: REQUEST_FRAMEBUFFER
	.short 0 # flags
	.long 24 # Size (2+2+4+4+4+4 = 20)
	.long 0 # default width
	.long 0 # default height
	.long 0 # default bpp
	.long 0

	# Padding to align to 8 bytes (2x4 bytes added above = 8 bytes, so already aligned)

	# End Tag (type 0)
	.long 0                                        # tag type: END
	.long 8                                        # size of end tag

multiboot_header_end:

# Allocate the initial stack.
.section .bootstrap_stack, "aw", @nobits
.align 4
stack_bottom:
.skip 16384 # 16 KiB
.global stack_top
.align 4
stack_top:

# Preallocate pages used for paging. Don't hard-code addresses and assume they
# are available, as the bootloader might have loaded its multiboot structures or
# modules there. This lets the bootloader know it must avoid the addresses.
.section .bss, "aw", @nobits
	.align 4096
.global boot_page_directory
boot_page_directory:
	.skip 4096
.global boot_page_table1
boot_page_table1:
	.skip 4096
.global boot_page_table2
boot_page_table2:
	.skip 4096
# Further page tables may be required if the kernel grows beyond 3 MiB.

# The kernel entry point.
.section .multiboot.text, "ax"
.global _start
.type _start, @function
_start:
	cli
	# Physical address of boot_page_table1.
	# TODO: I recall seeing some assembly that used a macro to do the
	#       conversions to and from physical. Maybe this should be done in this
	#       code as well?
	movl $(boot_page_table1 - 0xC0000000), %edi
	# First address to map is address 0.
	# TODO: Start at the first kernel page instead. Alternatively map the first
	#       1 MiB as it can be generally useful, and there's no need to
	#       specially map the VGA buffer.
	movl $0, %esi
	movl $2048, %ecx

1:
	# Only map the kernel.
	#cmpl $_kernel_start, %esi
	#jl 2f
	cmpl $(0xC0800000 - 0xC0000000), %esi
	jge 3f

	# Map physical address as "present, writable". Note that this maps
	# .text and .rodata as writable. Mind security and map them as non-writable.
	movl %esi, %edx
	orl $0x003, %edx
	movl %edx, (%edi)

2:
	# Size of page is 4096 bytes.
	addl $4096, %esi
	# Size of entries in boot_page_table1 is 4 bytes.
	addl $4, %edi
	# Loop to the next entry if we haven't finished.
	loop 1b

3:
	# The page table is used at both page directory entry 0 & 1 (virtually from 0x0
	# to 0x7FFFFF) (thus identity mapping the kernel) and page directory entry
	# 768 & 769 (virtually from 0xC0000000 to 0xC07FFFFF) (thus mapping it in the
	# higher half). The kernel is identity mapped because enabling paging does
	# not change the next instruction, which continues to be physical. The CPU
	# would instead page fault if there was no identity mapping.

	# Map the page table to both virtual addresses 0x00000000 and 0xC0000000.
	movl $(boot_page_table1 - 0xC0000000 + 0x003), (boot_page_directory - 0xC0000000) + 0
	movl $(boot_page_table2 - 0xC0000000 + 0x003), (boot_page_directory - 0xC0000000) + 4
	movl $(boot_page_table1 - 0xC0000000 + 0x003), (boot_page_directory - 0xC0000000) + (768 * 4)
	movl $(boot_page_table2 - 0xC0000000 + 0x003), (boot_page_directory - 0xC0000000) + (769 * 4)

	# Set cr3 to the address of the boot_page_directory.
	movl $(boot_page_directory - 0xC0000000), %ecx
	movl %ecx, %cr3

	# Enable paging and the write-protect bit.
	movl %cr0, %ecx
	orl $0x80000000, %ecx
	movl %ecx, %cr0

	# Jump to higher half with an absolute jump. 
	lea 4f, %ecx
	jmp *%ecx

.section .text

4:
	# At this point, paging is fully set up and enabled.
	movl $0, boot_page_directory + 0
	movl $0, boot_page_directory + 1

	# Reload crc3 to force a TLB flush so the changes to take effect.
	movl %cr3, %ecx
	movl %ecx, %cr3

	# Set up the stack.
	mov $stack_top, %esp

	push %eax
	push %ebx
	# Call global constructors
	call _init

	# Enter the high-level kernel.
	call kernel_main

	# Infinite loop if the system has nothing more to do.
	cli
1:	hlt
	jmp 1b
