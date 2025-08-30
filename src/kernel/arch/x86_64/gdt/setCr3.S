.global set_cr3
set_cr3:
	movq %rdi,%cr3
	ret