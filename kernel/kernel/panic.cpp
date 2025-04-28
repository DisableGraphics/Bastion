#include <kernel/kernel/panic.hpp>
#include <kernel/drivers/interrupts.hpp>
#include <kernel/assembly/inlineasm.h>
#include <stdio.h>

void kn::panic(const char *str) {
	clear();
	printf("Kernel panic: %s\nRebooting is recommended.\n", str);
	IDT::get().disable_interrupts();
	halt();
}