.global syscall
syscall:
	# Move syscall number into rax
	mov 8(%rsp),%rax
	mov %rcx,%r10
	syscall
	ret