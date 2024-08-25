#include <stdio.h>
void kerror(const char *msg) {
	printf("Error: %s\n", msg);
	__asm__ volatile("hlt");
}