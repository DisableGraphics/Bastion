#include <kernel/panic.hpp>
#include <kernel/interrupts.hpp>
#include <kernel/inlineasm.h>
#include <stdio.h>

void kn::panic(const char *str) {
	printf("Kernel panic: %s\n", str);
	IDT::get().disable_interrupts();
	halt();
}