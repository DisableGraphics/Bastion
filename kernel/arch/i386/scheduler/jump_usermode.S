.global jump_usermode
.extern test_user_function
jump_usermode:
    mov $((4 * 8) | 3), %ax     # Ring 3 data selector with bottom 2 bits set for ring 3
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs               # SS is handled by iret

    # Set up the stack frame iret expects
    mov %esp, %eax
    push $((4 * 8) | 3)        # Data selector
    push %eax                  # Current ESP
    pushf                      # EFLAGS
    push $((3 * 8) | 3)        # Code selector (ring 3 code with bottom 2 bits set for ring 3)
    push $test_user_function   # Instruction address to return to
    iret
