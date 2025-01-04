.global flush_tss
flush_tss:
    mov $((5 * 8) | 0), %ax  # Load selector into AX register (fifth 8-byte selector)
    ltr %ax                  # Load AX into Task Register
    ret                      # Return
