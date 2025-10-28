#1) clear the CR4.OSFXSR bit (bit 9) [ CR4 |= (1 << 9) ]
#2) clear the CR4.OSXMMEXCPT bit (bit 10) [ CR4 |= (1 << 10) ]

.global disable_sse
disable_sse:
	mov %cr4, %rax
	and $~(3 << 9), %rax
	mov %rax, %cr4
	ret