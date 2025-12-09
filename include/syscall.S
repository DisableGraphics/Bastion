.global syscall
syscall:
	# Move syscall number into rax
	mov 8(%rsp),%rax
	syscall
	ret