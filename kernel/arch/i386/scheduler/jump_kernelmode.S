.global jump_kernelmode
.extern test_kernel_function
jump_kernelmode:
    mov $((2 * 8) | 0), %ax     # Ring 0 data selector with bottom 2 bits set for ring 0
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs               # SS is handled by iret

    # Set up the stack frame iret expects
    mov %esp, %eax
    push $((2 * 8) | 0)        # Data selector
    push %eax                  # Current ESP
    pushf                      # EFLAGS
    push $((1 * 8) | 0)        # Code selector (ring 0 code with bottom 2 bits set for ring 0)
    push $test_kernel_function   # Instruction address to return to
    iret
