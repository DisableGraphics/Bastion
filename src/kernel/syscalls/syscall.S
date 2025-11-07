.extern syscall_handler_stage_1
.global syscall_handler
syscall_handler:
	mov %rcx,%r12
	mov %rax,%rcx
	call syscall_handler_stage_1
	mov %r12,%rcx
	sysret
