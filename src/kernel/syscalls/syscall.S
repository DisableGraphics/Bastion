.extern syscall_handler_stage_1
.global syscall_handler
syscall_handler:
	# Switch to kernel stack
	swapgs
	mov %rcx,%gs:24
	mov %rsp,%rcx
	mov %gs:8,%rsp
	push %rcx
	mov %gs:24,%rcx
	push %rcx
	push %r11
	swapgs
	# Prepare syscall
	mov %r10,%rcx
	push %rax
	call syscall_handler_stage_1
	add $8,%rsp
	# Switch from the kernel stack to user stack
	pop %r11
	pop %rcx
	pop %rdi
	mov %rdi,%rsp
	
	sysretq