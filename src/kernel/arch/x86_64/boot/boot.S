.global _start
_start:
    call kmain
loop:
	hlt
	jmp loop
