.global syscall
syscall:
	mov %rcx,%rax
	syscall
	ret