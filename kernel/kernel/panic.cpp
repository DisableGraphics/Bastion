#include <kernel/kernel/panic.hpp>
#include <kernel/drivers/interrupts.hpp>
#include <kernel/assembly/inlineasm.h>
#include <stdio.h>
#include <kernel/kernel/log.hpp>

void kn::panic(const char *str) {
	log(ERROR, "Kernel panic: %s\nRebooting is recommended.\n", str);
	__asm__ __volatile__("cli");
	halt();
}