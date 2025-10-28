.global save_sse
save_sse:
	fxsave (%rdi)
	ret