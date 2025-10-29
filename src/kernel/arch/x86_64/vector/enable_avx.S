# As per the Osdev WIKI:
# AVX needs to be enabled by the kernel before being used. Forgetting to do this will raise an #UD on the first AVX call. Both SSE and OSXSAVE must be enabled before allowing. Failing to do so will also produce an #UD.
# AVX is enabled by setting bit 2 of the XCR0 register. Bit 1 of XCR0 must also be set (indicating SSE support). 

.global enable_osxsave
enable_osxsave:
	mov %cr4,%rax
	# OR in: (1<<9) + (1<<18) = 512 + 262144 = 262656
	orq $262656, %rax
	mov %rax,%cr4

	xorl %edx, %edx
	movl $3, %eax
	xorl %ecx, %ecx
	xsetbv
	ret

.global enable_avx
enable_avx:
	pushq %rax
	pushq %rcx
	pushq %rdx

	xorl %ecx, %ecx
	xgetbv
	orl $7, %eax
	xsetbv

	popq %rdx
	popq %rcx
	popq %rax
	retq
