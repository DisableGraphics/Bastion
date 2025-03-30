#pragma once
#include <stdint.h>
#include <stdbool.h>
#define fence() __asm__ volatile ("":::"memory")

#ifdef __cplusplus
extern "C" {
#endif

#define MSR_MTRRphysBase0 0x200
#define MSR_MTRRphysMask0 0x201
#define IA32_MTRR_DEF_TYPE 0x2FF

// FAR PEEKx
uint32_t farpeekl(uint16_t sel, void *off);
// FAR POKEx
void farpokeb(uint16_t sel, void* off, uint8_t v);

// OUTx
void outb(uint16_t port, uint8_t val);
void outw(uint16_t port, uint16_t val);
void outl(uint16_t port, uint32_t val);

// INx
uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);

bool are_interrupts_enabled();

unsigned long save_irqdisable();
void irqrestore(unsigned long flags);

uint64_t rdtsc();

unsigned long read_cr0();
unsigned long read_cr2();
unsigned long read_cr3();
unsigned long read_cr4();
unsigned long read_cr8();

void invlpg(void* m);

void wrmsr(uint32_t msr_id, uint64_t msr_value);
uint64_t rdmsr(uint32_t msr_id);

void io_wait(void);

void halt(void);

void tlb_flush(void);

uint32_t get_eflags();
uint32_t get_esp();

#ifdef __cplusplus
}
#endif