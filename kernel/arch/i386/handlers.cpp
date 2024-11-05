#include <kernel/interrupts.hpp>
#include <stdio.h>

void IDT::generic_interrupt_handler(interrupt_frame* frame) {
	printf("Interrupt happened\n");
}

void IDT::division_by_zero_handler(interrupt_frame* frame) {
	printf("Division by zero\n");
}