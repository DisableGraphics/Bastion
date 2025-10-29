# As per Osdev Wiki, enabling SSE goes like this:

#1) clear the CR0.EM bit (bit 2) [ CR0 &= ~(1 << 2) ]
#2) set the CR0.MP bit (bit 1) [ CR0 |= (1 << 1) ]
#2.5) Set the CR0.TS bit so that it interrupts when the CPU tries to use sse
#3) set the CR4.OSFXSR bit (bit 9) [ CR4 |= (1 << 9) ]
#4) set the CR4.OSXMMEXCPT bit (bit 10) [ CR4 |= (1 << 10) ]

# And also from what I understand, enabling SSE will enable all SSEN options (SSE1, SSE2, SSE3, SSSE3, SSE4.1, SSE4.2, SSE4.A)

.global enable_sse
enable_sse:
	movq %cr0, %rax
	and $0xFFFB, %ax
	or $0x2, %ax
	or $0x8, %ax
	movq %rax,%cr0
	movq %cr4, %rax
	or $(3 << 9), %ax
	movq %rax, %cr4
	ret