.global save_avx
save_avx:
	xsave (%rdi)
	ret