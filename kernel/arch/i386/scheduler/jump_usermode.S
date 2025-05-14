.global jump_usermode
# def: jump_usermode(void* fn)
jump_usermode:
	mov (4 * 8) | 3, %ax
	mov %ax, %ds
	mov %ax, %es 
	mov %ax, %fs
	mov %ax, %gs

	mov %esp, %eax
	push (4 * 8) | 3
	push %eax
	pushf
	push (3 * 8) | 3
	push 20(%esp)
	iret
