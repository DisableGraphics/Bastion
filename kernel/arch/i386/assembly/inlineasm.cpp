#include <kernel/assembly/inlineasm.h>

uint32_t farpeekl(uint16_t sel, void *off) {
	uint32_t ret;
	__asm__ ( "push %%fs\n\t"
		  "mov  %1, %%fs\n\t"
		  "mov  %%fs:(%2), %0\n\t"
		  "pop  %%fs"
		  : "=r"(ret) : "g"(sel), "r"(off) );
	return ret;
}

void farpokeb(uint16_t sel, void* off, uint8_t v) {
	__asm__ ( "push %%fs\n\t"
		  "mov  %0, %%fs\n\t"
		  "movb %2, %%fs:(%1)\n\t"
		  "pop %%fs"
		  : : "g"(sel), "r"(off), "r"(v) );
}

void outb(uint16_t port, uint8_t val) {
	__asm__ volatile ( "outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

void outw(uint16_t port, uint16_t val) {
	__asm__ volatile ( "outw %w0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

void outl(uint16_t port, uint32_t val) {
	__asm__ volatile ( "outl %d0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

uint8_t inb(uint16_t port) {
	uint8_t ret;
	__asm__ volatile ( "inb %w1, %b0"
				   : "=a"(ret)
				   : "Nd"(port)
				   : "memory");
	return ret;
}

uint16_t inw(uint16_t port) {
	uint16_t ret;
	__asm__ volatile ( "inw %w1, %w0"
				   : "=a"(ret)
				   : "Nd"(port)
				   : "memory");
	return ret;
}

uint32_t inl(uint16_t port) {
	uint32_t ret;
	__asm__ volatile ( "inl %w1, %d0"
				   : "=a"(ret)
				   : "Nd"(port)
				   : "memory");
	return ret;
}

bool are_interrupts_enabled() {
	unsigned long flags;
	__asm__ volatile ( "pushf\n\t"
				   "pop %0"
				   : "=g"(flags) );
	return flags & (1 << 9);
}

unsigned long save_irqdisable(void) {
	unsigned long flags;
	__asm__ volatile ("pushf\n\tcli\n\tpop %0" : "=r"(flags) : : "memory");
	return flags;
}

void irqrestore(unsigned long flags) {
	__asm__ ("push %0\n\tpopf" : : "rm"(flags) : "memory","cc");
}

uint64_t rdtsc() {
	uint64_t ret;
	__asm__ volatile ( "rdtsc" : "=A"(ret) );
	return ret;
}

unsigned long read_cr0(void) {
	unsigned long val;
	__asm__ volatile ( "mov %%cr0, %0" : "=r"(val) );
	return val;
}
unsigned long read_cr2(void) {
	unsigned long val;
	__asm__ volatile ( "mov %%cr2, %0" : "=r"(val) );
	return val;
}
unsigned long read_cr3(void) {
	unsigned long val;
	__asm__ volatile ( "mov %%cr3, %0" : "=r"(val) );
	return val;
}
unsigned long read_cr4(void) {
	unsigned long val;
	__asm__ volatile ( "mov %%cr4, %0" : "=r"(val) );
	return val;
}
unsigned long read_cr8(void) {
	unsigned long val;
	__asm__ volatile ( "mov %%cr8, %0" : "=r"(val) );
	return val;
}

void invlpg(void* m) {
	/* Clobber memory to avoid optimizer re-ordering access before invlpg, which may cause nasty bugs. */
	asm volatile ( "invlpg (%0)" : : "b"(m) : "memory" );
}

void wrmsr(uint32_t msr_id, uint64_t msr_value) {
	asm volatile ( "wrmsr" : : "c" (msr_id), "A" (msr_value) );
}
uint64_t rdmsr(uint32_t msr_id) {
	uint64_t msr_value;
	asm volatile ( "rdmsr" : "=A" (msr_value) : "c" (msr_id) );
	return msr_value;
}

void io_wait(void) {
	outb(0x80, 0);
}

void halt(void) {
	__asm__ __volatile__("hlt");
}

void tlb_flush() {
	__asm__ __volatile__("movl	%cr3,%eax\nmovl	%eax,%cr3");
}