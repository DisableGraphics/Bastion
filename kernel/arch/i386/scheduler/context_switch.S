.section .text
.global ctx_swtch

ctx_swtch:
    pushfl                  # Save flags
    cli                     # Disable interrupts
    pusha                   # Save all general-purpose registers

    movl 40(%esp), %eax      # Load address of old_sp into eax
    movl %esp, (%eax)       # Save current stack pointer to *old_sp

    movl 44(%esp), %eax     # Load address of new_sp into eax
    movl %eax, %esp         # Load new stack pointer from *new_sp

    popa                    # Restore all general-purpose registers
    popfl                   # Restore flags
    sti                     # Enable interrupts

    ret                     # Return to the new context
