.global restore_avx
restore_avx:
	xrstor (%rdi)
	ret