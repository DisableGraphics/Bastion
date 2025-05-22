.global jump_usermode
# definition: jump_usermode(void* fn, size_t esp)
jump_usermode:
	cli
	mov 4(%esp), %ecx
	mov 8(%esp), %edx
	mov $((4 * 8) | 3), %ax
	mov %ax, %ds
	mov %ax, %es 
	mov %ax, %fs
	mov %ax, %gs

	mov %edx, %eax
	push $((4 * 8) | 3)
	push %eax
	pushf
	orl $0x200,(%esp)
	push $((3 * 8) | 3)
	push %ecx
	iret
