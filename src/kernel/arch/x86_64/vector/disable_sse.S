#1) set the ts bit in cr0

.global disable_sse
disable_sse:
	mov %cr0, %rax
	or $(1 << 3),%rax
	mov %rax, %cr0
	ret