.extern syscall_handler_stage_1
.global syscall_handler
syscall_handler:
	# I'm being a smarty pants: r12 is a callee-saved register 
	# which means that syscall_handler_stage_1 is the one that manages it.
	mov %rcx,%r12
	# Move original RCX to where it was
	mov %rax,%rcx
	call syscall_handler_stage_1
	# Move return address to RCX so that we can go back
	mov %r12,%rcx
	sysret
