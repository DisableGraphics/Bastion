#1) clear the ts bit in cr0

.global clear_sse
clear_sse:
	clts
	ret