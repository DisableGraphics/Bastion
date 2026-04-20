.global switch_task
# void switch_task(Task** current_thread, Task *next_thread, tss_t* tss, bool current_thread_deleted);
# System V x86_64 ABI: 
#	rdi = &current_thread
#	rsi = next_thread
#	rdx = tss
#	rcx = current_thread_deleted
#	r8 = per_process_table

switch_task:
	# rdi = current_thread
	mov (%rdi), %r10        # r10 = *current_thread (pointer to TCB of current task)
	cmp $0,%rcx				# Check if current_thread has been deleted
	jne .no_current			# If it is, we don't need to save the register state
	# Save previous task's state (callee-saved regs only)
	# push %rbx
	# push %rbp
	# push %r12
	# push %r13
	# push %r14
	# push %r15
	# pushfq
	# Saved registers appear from offset 24
	# And are in this order: r15 (24), r14 (32), r13 (40), r12 (48), rbp (56), rbx (64), rflags (72)
	movq %r15,24(%r10)
	movq %r14,32(%r10)
	movq %r13,40(%r10)
	movq %r12,48(%r10)
	movq %rbp,56(%r10)
	movq %rbx,64(%r10)
	# Fucking rflags
	pushfq
	pop %r15
	movq %r15,72(%r10)

	movq %rsp, (%r10)        # Save kernel stack pointer (RSP) into TCB (If current_task isn't deleted of course)
.no_current:
	# Load next task's state
	movq %rsi, (%rdi)        # *current_thread = next_thread

	movq (%rsi), %rsp        # Load RSP for next task's kernel stack from TCB
	movq 8(%rsi), %rax       # rax = page directory (PML4) of next task
	movq 16(%rsi), %rbx      # rbx = new kernel stack top for TSS.ESP0

	# Update TSS->rsp0
	movq %rbx, 4(%rdx)       # adjust rsp0 field in TSS

	# Switch address space if needed
	movq %cr3, %r10
	cmp %rax, %r10
	je .doneVAS
	movq %rax, %cr3
.doneVAS:

	# Restore callee-saved regs
	# popfq
	# pop %r15
	# pop %r14
	# pop %r13
	# pop %r12
	# pop %rbp
	# pop %rbx

	movq 24(%rsi), %r15
	movq 32(%rsi), %r14
	movq 40(%rsi), %r13
	movq 48(%rsi), %r12
	movq 56(%rsi), %rbp
	movq 64(%rsi), %rbx

	# Save RSP0 into per-process data structure
	mov %rsp,8(%r8)

	push %rsi
	push %r8
	call unlock_scheduler_from_context_switch
	pop %r8
	pop %rsi

	# Now it's safe to maybe enable interrupts again
	movq 72(%rsi), %r8
	push %r8
	popfq
	ret
