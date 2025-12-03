.global jump_to_ring3
# Yes, I'm being clever using r15 & r14 to save return address and user mode stack, respectively
jump_to_ring3:
	mov %r15,%rcx # Setup return address
	mov $0x202,%r11 # Enable interrupts plus that bit that has to be active
	mov %r14,%rsp # User mode stack
	sysretq