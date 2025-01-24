.global switch_task
#C declaration:
#   void switch_task(Task** current_thread, Task *next_thread);
#
#WARNING: Caller is expected to disable IRQs before calling, and enable IRQs again after function returns

switch_task:
	mov 4(%esp),%eax # eax = address of current thread
    #Save previous task's state

    #Notes:
    #  For cdecl# EAX, ECX, and EDX are already saved by the caller and don't need to be saved again
    #  EIP is already saved on the stack by the caller's "CALL" instruction
    #  The task isn't able to change CR3 so it doesn't need to be saved
    #  Segment registers are constants (while running kernel code) so they don't need to be saved

    push %ebx
    push %esi
    push %edi
    push %ebp
	pushf

    mov %eax,%edi    #edi = address of the previous task's "thread control block"
	mov (%edi),%ecx
    mov %esp,(%ecx)         #Save ESP for previous task's kernel stack in the thread's TCB

    #Load next task's state

    mov 28(%esp),%esi         #esi = address of the next task's "thread control block" (parameter passed on stack)
	mov %esi,(%edi)
    mov %esi,(%eax)    		#Current task's TCB is the next task TCB

    mov (%esi),%esp         #Load ESP for next task's kernel stack from the thread's TCB
    mov 4(%esi),%eax         #eax = address of page directory for next task
    mov 8(%esi),%ebx        #ebx = address for the top of the next task's kernel stack
	mov $tss,%ecx
    mov %ebx,4(%ecx)            #Adjust the ESP0 field in the TSS (used by CPU for for CPL=3 -> CPL=0 privilege level changes)
    mov %cr3,%ecx                   #ecx = previous task's virtual address space

    cmp %eax,%ecx                   #Does the virtual address space need to being changed?
    je .doneVAS                   # no, virtual address space is the same, so don't reload it and cause TLB flushes
    mov %eax,%cr3                   # yes, load the next task's virtual address space
.doneVAS:

	or $0x200,(%esp) # Interrupt flag
	popf
    pop %ebp
    pop %edi
    pop %esi
    pop %ebx

    ret                           #Load next task's EIP from its kernel stack
