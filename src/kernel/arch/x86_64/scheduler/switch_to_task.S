.global switch_task
# void switch_task(Task** current_thread, Task *next_thread, tss_t* tss, bool current_thread_deleted);
# System V x86_64 ABI: 
#	rdi = &current_thread
#	rsi = next_thread
#	rdx = tss
#	rcx = current_thread_deleted

switch_task:
	# rdi = current_thread
	mov (%rdi), %r10        # rcx = *current_thread (pointer to TCB of current task)
	cmp $0,%rcx				# Check if current_thread has been deleted
	jne .no_current			# If it is, we don't need to save the register state
	# Save previous task's state (callee-saved regs only)
	push %rbx
	push %rbp
	push %r12
	push %r13
	push %r14
	push %r15
	push %fs
	pushfq
	mov %rsp, (%r10)        # Save kernel stack pointer (RSP) into TCB (If current_task isn't deleted of course)
.no_current:
	# Load next task's state
	mov %rsi, (%rdi)        # *current_thread = next_thread

	mov (%rsi), %rsp        # Load RSP for next task's kernel stack from TCB
	mov 8(%rsi), %rax       # rax = page directory (PML4) of next task
	mov 16(%rsi), %rbx      # rbx = new kernel stack top for TSS.ESP0

	# Update TSS->rsp0
	mov %rbx, 4(%rdx)       # adjust rsp0 field in TSS

	# Switch address space if needed
	mov %cr3, %r10
	cmp %rax, %r10
	je .doneVAS
	mov %rax, %cr3
.doneVAS:

	# Restore callee-saved regs
	popfq
	pop %fs
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %rbp
	pop %rbx
	sti

	ret
