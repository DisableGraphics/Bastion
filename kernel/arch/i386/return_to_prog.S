.text
.globl return_to_prog
return_to_prog:
	movw registers+44, %ss
	movl registers+16, %esp
	movl registers+4, %ecx
	movl registers+8, %edx
	movl registers+12, %ebx
	movl registers+20, %ebp
	movl registers+24, %esi
    movl registers+28, %edi
    movw registers+48, %ds
    movw registers+52, %es
    movw registers+56, %fs
    movw registers+60, %gs
    movl registers+36, %eax
    pushl %eax  /* saved eflags */
    movl registers+40, %eax
    pushl %eax  /* saved cs */
    movl registers+32, %eax
    pushl %eax  /* saved eip */
    movl registers, %eax
/* use iret to restore pc and flags together so
   that trace flag works right.  */
    iret