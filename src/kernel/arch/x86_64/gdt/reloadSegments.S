.global reloadSegments
reloadSegments:
	pushq $0x08
	leaq .reload_CS(%rip), %rax
	pushq %rax
	lretq
.reload_CS:
	movw $0x10,%ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	movw %ax, %ss
	retq