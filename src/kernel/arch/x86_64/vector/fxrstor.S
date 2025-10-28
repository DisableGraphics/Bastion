.global restore_sse
restore_sse:
	fxrstor (%rdi)
	ret