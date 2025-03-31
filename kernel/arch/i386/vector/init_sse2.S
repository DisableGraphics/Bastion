.global init_sse2
init_sse2:
	mov %cr0,%eax
	and $0xFFFB,%ax		#clear coprocessor emulation CR0.EM
	or $0x2,%ax			#set coprocessor monitoring  CR0.MP
	mov %eax,%cr0
	mov %cr4,%eax
	or $(3 << 9),%ax	#set CR4.OSFXSR and CR4.OSXMMEXCPT at the same time
	mov %eax,%cr4
	fninit
	ret