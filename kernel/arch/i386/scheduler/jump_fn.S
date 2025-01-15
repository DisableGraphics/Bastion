.global jump_fn

/* C signature: 
	void jump_fn(uint32_t **old_esp, 
		uint32_t *new_esp, 
		void (*fn)(), 
		uint8_t code_segment_selector, 
		uint8_t data_segment_selector, 
		uint8_t ring);
*/
jump_fn:
	# Save old ESP to *old_esp
	mov %esp, %eax                  # Get current ESP into EAX
	mov 4(%esp), %ecx               # Load *old_esp (1st argument)
	mov %eax, (%ecx)                # Store ESP at the address pointed by *old_esp

	# Prepare for the jump to the kernel function
	xor %ecx,%ecx					# Clear ECX register
	mov 16(%esp), %cl               # Load code_segment_selector (4th argument)
	xor %edx,%edx					# Clear EDX register 
	mov 20(%esp), %dl               # Load data_segment_selector (5th argument)
	mov 24(%esp), %bl               # Load ring (6th argument)

	# Calculate selectors with the ring
	shl $3, %cl                     # Shift code_segment_selector left by 3 (convert to GDT entry)
	or %bl, %cl                     # Add ring to code segment selector
	shl $3, %dl                     # Shift data_segment_selector left by 3 (convert to GDT entry)
	or %bl, %dl                     # Add ring to data segment selector

	# Load data segment selectors
	mov %dx, %ax                    # Data segment selector in AX
	mov %ax, %ds                    # Load DS
	mov %ax, %es                    # Load ES
	mov %ax, %fs                    # Load FS
	mov %ax, %gs                    # Load GS

	# Load new_esp into ESP
	mov 8(%esp), %ebx               # Load new_esp (2nd argument)
	mov 12(%esp), %eax              # Load fn (3rd argument) into EAX
	mov %ebx, %esp                # Set ESP to new_esp

	# Set up the stack frame for iret
	push %edx                       # Push data segment selector (DS) onto the stack
	push %esp                       # Push current ESP onto the stack
	
	pushf                           # Push EFLAGS onto the stack
	or $0x200, (%esp)               # Interrupt flag
	
	push %ecx                       # Push code segment selector (CS) onto the stack
	push %eax                       # Push the function address (fn) onto the stack
	# Perform the context switch
	iret